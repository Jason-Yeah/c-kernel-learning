const reject = (v, message) => {
  if (!v) throw new Error('Byte-order evidence rejected: ' + message);
};
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
export function validateByteOrder(t) {
  reject(t?.id === 'byte-order' && t.schemaVersion === 1, 'schema');
  const p = t.provenance, samples = t.runs?.single?.samples;
  reject(p?.source?.content?.includes('const unsigned char *view') && /^[a-f0-9]{64}$/.test(p.source.sha256), 'complete source/hash');
  reject(/^[a-f0-9]{64}$/.test(p.binary?.sha256 || '') && p.compilerCommand?.includes('-O0'), 'build provenance');
  reject(p.architecture?.includes('x86-64') && /off/.test(p.debuggerASLR || ''), 'actual x86-64 with ASLR');
  reject(samples?.length === 12 && t.exitCode === 0, 'bounded checkpoints / normal exit');
  const expectedEvents = ['ready', ...[0,1,2,3].map(i => 'read-'+i), ...[0,1,2,3].map(i => 'encode-'+i), 'native-read', 'raw-read', 'decoded'];
  reject(same(samples.map(s => s.event), expectedEvents), 'event sequence');
  const first = samples[0], word = first.fields.word;
  reject(Number.isInteger(word) && word > 0 && word <= 0xFFFFFFFF, 'uint32 source');
  const little = [0,1,2,3].map(i => (word >>> (8*i)) & 255), big = [...little].reverse();
  reject(same(first.objects.word.bytes, little), 'actual native little-endian representation');
  reject(same(first.objects.be_encoded.bytes, [204,204,204,204]), 'explicit CC initialization');
  let before = [...first.objects.be_encoded.bytes];
  samples.forEach((s, i) => {
    reject(s.index === i && s.pid === first.pid && s.pid > 0, 'single execution identity');
    reject(s.phase === [1,2,2,2,2,3,3,3,3,4,5,6][i], 'actual checkpoint phase');
    reject(s.fields.native_read === (i < 9 ? 0 : word), 'native result lifetime');
    reject(s.fields.decoded_be === (i < 11 ? 0 : word), 'decoded result lifetime');
    reject(s.fields.word === word && same(s.objects.word, first.objects.word), 'source word preserved');
    reject(s.objects.be_encoded.address === first.objects.be_encoded.address, 'fixed BE object');
    for (const name of ['word', 'be_encoded']) reject(/^0x[0-9A-F]+$/.test(s.objects[name].address) && s.objects[name].bytes.length === 4, 'object addresses / capacity');
    reject(s.source?.line > 0 && s.callsite?.line > 0 && s.registers?.rip && s.registers.rsp, 'source/register observations');
    for (const c of ['info registers rip rsp rbp rax rdi rsi rdx eflags', 'p/x word', 'x/4bx &word', 'x/4bx be_encoded', 'p/x native_read', 'p/x raw_read', 'p/x decoded_be']) {
      reject(s.commands?.some(x => x.command === c && x.output), 'missing original GDB command ' + c);
    }
    if (s.event.startsWith('read-')) {
      const n = s.fields.read_index;
      reject(n === i - 1 && s.fields.probe_byte === little[n], 'actual byte read');
    }
    if (s.event.startsWith('encode-')) {
      const n = s.fields.encode_index;
      reject(n === i - 5, 'encode index');
      before[n] = big[n];
    }
    reject(same(s.objects.be_encoded.bytes, before), 'exact byte commit, no extra write');
    for (const [command, expected] of [['x/4bx &word', s.objects.word.bytes], ['x/4bx be_encoded', s.objects.be_encoded.bytes]]) {
      const output = s.commands.find(c => c.command === command).output;
      const bytes = [...output.matchAll(/\b0x([0-9a-fA-F]{1,2})\b/g)].map(m => parseInt(m[1],16));
      reject(same(bytes, expected), 'original memory output mismatch');
    }
    for (const [command, field] of [['p/x word','word'],['p/x native_read','native_read'],['p/x raw_read','raw_read'],['p/x decoded_be','decoded_be']]) {
      const output = s.commands.find(c => c.command === command).output;
      reject(Number(output.match(/=\s*(0x[0-9a-fA-F]+)/)?.[1]) === s.fields[field], 'original scalar output mismatch');
    }
  });
  reject(first.objects.word.address !== first.objects.be_encoded.address, 'separate native/BE objects');
  reject(first.fields.native_read === 0 && first.fields.raw_read === 0 && first.fields.decoded_be === 0, 'known initial scalar values');
  reject(samples[9].fields.native_read === word && samples[9].fields.raw_read === 0, 'native memcpy read');
  const rawValue = big.reduce((v, b, i) => v + b * 2**(8*i), 0);
  reject(samples[10].fields.raw_read === rawValue && samples[10].fields.decoded_be === 0, 'native reading of BE encoding');
  reject(samples[11].fields.decoded_be === word && same(samples[11].objects.be_encoded.bytes, big), 'explicit BE decoding');
  reject(t.stdout?.includes('word=12345678 native=12345678 raw=78563412 decoded=12345678'), 'actual stdout');
  return {samples: samples.length, events: expectedEvents.length, word, little, big, rawValue};
}
