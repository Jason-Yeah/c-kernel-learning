"""Capture only the reviewed byte-order.c; no arbitrary program execution."""
import gdb
import json
import traceback

LOG, EXITS = [], []
REGS = ('rip', 'rsp', 'rbp', 'rax', 'rdi', 'rsi', 'rdx', 'eflags')
FIELDS = ('word', 'probe_byte', 'read_index', 'encode_index', 'native_read', 'raw_read', 'decoded_be')


def cmd(text):
    output = gdb.execute(text, to_string=True).strip()
    LOG.append('(gdb) ' + text + '\n' + output)
    return {'command': text, 'output': output}


def hx(v):
    return '0x%X' % (int(v) & ((1 << 64) - 1))


def mem(address, count):
    return list(bytes(gdb.selected_inferior().read_memory(address, count)))


try:
    for c in ('set pagination off', 'set confirm off', 'set style enabled off', 'set disable-randomization off', 'file .trace-build/byte-order/program'):
        cmd(c)
    cmd('start > .trace-build/byte-order/stdout.txt')
    architecture = gdb.selected_frame().architecture().name()
    gdb.Breakpoint('checkpoint', internal=True)
    samples = []
    for index in range(20):
        cmd('continue')
        frame = gdb.selected_frame()
        if frame.name() != 'checkpoint':
            raise RuntimeError('Expected reviewed observer function')
        # Arguments are inspected only after GDB reaches its source breakpoint/prologue.
        phase = int(gdb.parse_and_eval('phase'))
        fields = {k: int(gdb.parse_and_eval(k)) for k in FIELDS}
        objects = {}
        for name in ('word', 'be_encoded'):
            address = int(gdb.parse_and_eval('&' + name))
            objects[name] = {'address': hx(address), 'bytes': mem(address, 4)}
        sal = frame.find_sal()
        caller = frame.older()
        callsite = caller.find_sal()
        registers = {k: hx(gdb.parse_and_eval('$' + k)) for k in REGS}
        commands = [cmd(c) for c in (
            'info registers ' + ' '.join(REGS), 'p/u phase',
            'p/x &word', 'p/x word', 'x/4bx &word',
            'p/u read_index', 'p/x probe_byte', 'p/u encode_index',
            'p/x &be_encoded', 'x/4bx be_encoded',
            'p/x native_read', 'p/x raw_read', 'p/x decoded_be')]
        event = ('ready' if phase == 1 else
                 'read-%d' % fields['read_index'] if phase == 2 else
                 'encode-%d' % fields['encode_index'] if phase == 3 else
                 {4: 'native-read', 5: 'raw-read', 6: 'decoded'}[phase])
        samples.append({'index': index, 'event': event, 'phase': phase,
                        'pid': gdb.selected_inferior().pid, 'fields': fields,
                        'objects': objects, 'registers': registers, 'commands': commands,
                        'source': {'file': sal.symtab.filename, 'line': sal.line, 'function': frame.name()},
                        'callsite': {'file': callsite.symtab.filename, 'line': callsite.line, 'function': caller.name()}})
        if phase == 6:
            break
    else:
        raise RuntimeError('Bound exceeded, no fallback')
    cmd('delete breakpoints')
    gdb.events.exited.connect(lambda e: EXITS.append(e.exit_code))
    cmd('continue')
    if EXITS != [0]:
        raise RuntimeError('Expected normal exit 0, got ' + repr(EXITS))
    with open('.trace-build/byte-order/stdout.txt', encoding='utf8') as f:
        stdout = f.read()
    with open('.trace-build/byte-order/raw.json', 'w', encoding='utf8') as f:
        json.dump({'samples': samples, 'exitCode': 0, 'stdout': stdout,
                   'architecture': architecture, 'gdb': gdb.VERSION,
                   'randomization': cmd('show disable-randomization')['output'],
                   'gdbText': '\n\n'.join(LOG)}, f, indent=2)
except Exception:
    gdb.write(traceback.format_exc(), gdb.STDERR)
    gdb.execute('quit 1')
