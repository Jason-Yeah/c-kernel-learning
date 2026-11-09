export const integerEvents = ['ready','and','or','xor','not','promotion','left','right','shift-guard','representation','invert8','negate8','extend','truncate','unsigned-wrap','signed-guard','promoted-add'];
const initial = {bits:182,mask:15,small:182,shift_count:32,u8:214,s8:-42,magnitude:42,small_max:127,small_min:-128,wide_unsigned:4660,wrap_lhs:4294967295,u8_max:255,signed_lhs:2147483647};
const changes = [{},{r_and:6,r_logical:1,r_logical_not:0},{r_or:191},{r_xor:185},{r_not:4294967113},{promoted_not:-183,narrow_not:73,small_shift:46592},{r_left:728},{r_right:45},{shift_rejected:1},{same_encoding:1},{inverted8:213},{negated8:214},{zero_extended:214,sign_extended:-42},{truncated:52},{wrapped:0,promoted_256:256,wrapped8:0},{wide_sum:2147483648,signed_rejected:1},{promoted_add:128,promoted_negmin:128}];
const widths = {
  bits:32,mask:32,small:8,r_and:32,r_or:32,r_xor:32,r_not:32,r_left:32,r_right:32,guarded_shift:32,
  r_logical:32,r_logical_not:32,promoted_not:32,small_shift:32,narrow_not:8,shift_count:32,shift_rejected:32,
  u8:8,s8:8,magnitude:8,inverted8:8,negated8:8,small_max:8,small_min:8,same_encoding:32,
  zero_extended:16,sign_extended:16,wide_unsigned:32,truncated:8,wrap_lhs:32,wrapped:32,u8_max:8,
  promoted_256:32,wrapped8:8,signed_lhs:32,signed_result:32,wide_sum:64,signed_rejected:32,promoted_add:32,promoted_negmin:32
};
const signed = new Set('r_logical r_logical_not promoted_not small_shift shift_rejected s8 small_max small_min same_encoding sign_extended promoted_256 signed_lhs signed_result wide_sum signed_rejected promoted_add promoted_negmin'.split(' '));
const fail = message => {throw new Error('Integer-bits evidence rejected: '+message);};
const require = (yes,message) => {if(!yes)fail(message);};
export function validateIntegerBits(t) {
  require(t?.schemaVersion===1 && t.id==='integer-bits','schema');
  const p=t.provenance;
  require(p?.kind==='gdb-plus-model' && p.source?.path==='experiments/integer-bits.c' && p.source.content.includes('int main(void)'),'complete reviewed C');
  for(const artifact of [p.source,p.binary]) require(/^[0-9a-f]{64}$/.test(artifact.sha256),'hash');
  require(/x86-64/.test(p.architecture) && p.kernelASLR==='2' && /off/.test(p.debuggerASLR),'recorded host / ASLR');
  require(t.exitCode===0 && t.sanitizer?.exitCode===0 && t.sanitizer.stderr==='' && t.sanitizer.stdout===t.stdout,'normal run and clean actual UBSan run');
  require(t.sanitizer.command.includes('-fsanitize=undefined') && t.sanitizer.command.includes('-fno-sanitize-recover=undefined'),'UBSan command');
  const samples=t.runs?.single?.samples;
  require(samples?.length===17,'17 observations');
  const state=Object.fromEntries(Object.keys(widths).map(k=>[k,initial[k]??0]));
  const addresses={}; let pid;
  samples.forEach((s,i)=>{
    require(s.index===i && s.phase===i+1 && s.event===integerEvents[i],'event order #'+i);
    require(Number.isInteger(s.pid) && s.pid>0 && (!pid||s.pid===pid),'one real PID');pid=s.pid;
    Object.assign(state,changes[i]);
    require(s.callsite?.file===p.source.path && s.callsite.function==='main' && Number.isInteger(s.callsite.line),'actual source');
    for(const [name,width] of Object.entries(widths)) {
      require(Number.isSafeInteger(s.fields?.[name]) && s.fields[name]===state[name],`${s.event}/${name} state`);
      const o=s.objects?.[name];
      const typeName=width===8?(signed.has(name)?'signed char':'unsigned char'):(signed.has(name)?'':'unsigned ')+({16:'short',32:'int',64:'long'}[width]);
      require(o?.width===width && o.signed===signed.has(name) && o.type===typeName,'actual Linux x86-64 type / width '+name);
      require(/^0x[0-9A-F]+$/.test(o.address) && (!addresses[name]||o.address===addresses[name]),'stable address '+name);addresses[name]=o.address;
      require(Array.isArray(o.bytes) && o.bytes.length===width/8 && o.bytes.every(b=>Number.isInteger(b)&&b>=0&&b<=255),'byte capacity '+name);
      let raw=o.bytes.reduce((n,b,j)=>n+(BigInt(b)<<BigInt(8*j)),0n);
      if(o.signed && (raw&(1n<<BigInt(width-1))))raw-=1n<<BigInt(width);
      require(raw===BigInt(s.fields[name]),'actual representation '+name);
      const scalar=s.commands?.find(c=>c.command===(o.signed?'p/d ':'p/u ')+name);
      require(scalar && new RegExp('=\\s*'+s.fields[name]+'(?:\\s|$)').test(scalar.output),'raw scalar '+name);
      const address=s.commands.find(c=>c.command==='p/x &'+name);
      require(address && address.output.toLowerCase().includes(o.address.toLowerCase()),'raw address '+name);
      const memory=s.commands.find(c=>c.command===`x/${width/8}bx &${name}`);
      const bytes=memory?.output.split(':').slice(1).join(':').match(/\b0x[0-9a-f]{2}\b/gi)?.map(v=>parseInt(v,16));
      require(bytes?.length===o.bytes.length && bytes.every((b,j)=>b===o.bytes[j]),'raw bytes '+name);
    }
    require(new Set(Object.values(s.objects).map(o=>o.address)).size===Object.keys(widths).length,'distinct C objects');
    const registerOutput=s.commands.find(c=>c.command==='info registers rip rsp rbp rax rdi rsi rdx eflags')?.output;
    require(typeof registerOutput==='string','raw registers');
    for(const r of ['rip','rsp','rbp','rax','rdi','rsi','rdx','eflags']){
      require(/^0x[0-9A-F]+$/.test(s.registers?.[r]),'register format');
      const match=registerOutput.match(new RegExp('^'+r+'\\s+(0x[0-9a-f]+)','im'));
      require(match && BigInt(match[1])===BigInt(s.registers[r]),'raw register '+r);
    }
  });
  const expectedTypes={'bits & mask':[32,false],'bits && mask':[32,true],'~bits':[32,false],'~small':[32,true],'small << 8':[32,true],'+small':[32,true],'small_max + 1':[32,true],'-small_min':[32,true],'wrap_lhs + 1u':[32,false],'(int64_t)signed_lhs + 1':[64,true]};
  require(t.types?.length===Object.keys(expectedTypes).length,'promotion evidence');
  for(const [expression,[width,isSigned]] of Object.entries(expectedTypes)) {
    const item=t.types.find(v=>v.expression===expression);
    require(item?.width===width && item.signed===isSigned && item.evidence?.command==='whatis '+expression && item.evidence.output==='type = '+item.type,'actual expression type '+expression);
  }
  require(t.stdout.includes('AND=00000006 OR=000000BF XOR=000000B9 NOT=FFFFFF49 LEFT=000002D8 RIGHT=0000002D') && t.stdout.includes('rejected_signed=1 promoted_add=128 negmin=128'),'stdout');
  return {samples:samples.length,events:integerEvents.length,objects:Object.keys(widths).length,runs:1};
}
