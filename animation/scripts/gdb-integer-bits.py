"""Capture only the reviewed integer-bits.c; invalid C operations are guarded."""
import gdb
import json
import traceback

LOG, EXITS = [], []
FIELDS = ('bits mask small r_and r_or r_xor r_not r_left r_right guarded_shift '
          'r_logical r_logical_not promoted_not small_shift narrow_not shift_count shift_rejected '
          'u8 s8 magnitude inverted8 negated8 small_max small_min same_encoding '
          'zero_extended sign_extended wide_unsigned truncated wrap_lhs wrapped '
          'u8_max promoted_256 wrapped8 signed_lhs signed_result wide_sum signed_rejected '
          'promoted_add promoted_negmin').split()
REGS = ('rip', 'rsp', 'rbp', 'rax', 'rdi', 'rsi', 'rdx', 'eflags')
EVENTS = ('ready', 'and', 'or', 'xor', 'not', 'promotion', 'left', 'right',
          'shift-guard', 'representation', 'invert8', 'negate8', 'extend', 'truncate',
          'unsigned-wrap', 'signed-guard', 'promoted-add')
EXPRESSIONS = ('bits & mask', 'bits && mask', '~bits', '~small', 'small << 8',
               '+small', 'small_max + 1', '-small_min', 'wrap_lhs + 1u',
               '(int64_t)signed_lhs + 1')


def cmd(text):
    output = gdb.execute(text, to_string=True).strip()
    LOG.append('(gdb) ' + text + '\n' + output)
    return {'command': text, 'output': output}


def hx(value):
    return '0x%X' % (int(value) & ((1 << 64) - 1))


try:
    for c in ('set pagination off', 'set confirm off', 'set style enabled off',
              'set disable-randomization off', 'file .trace-build/integer-bits/program'):
        cmd(c)
    cmd('start > .trace-build/integer-bits/stdout.txt')
    architecture = gdb.selected_frame().architecture().name()
    types = []
    for expr in EXPRESSIONS:
        value = gdb.parse_and_eval(expr)
        ctype = value.type.strip_typedefs()
        types.append({'expression': expr, 'type': str(ctype), 'width': ctype.sizeof * 8,
                      'signed': int(gdb.Value(-1).cast(ctype)) < 0,
                      'evidence': cmd('whatis ' + expr)})
    gdb.Breakpoint('checkpoint', internal=True)
    samples = []
    for index, event in enumerate(EVENTS):
        cmd('continue')
        frame = gdb.selected_frame()
        if frame.name() != 'checkpoint' or int(gdb.parse_and_eval('phase')) != index + 1:
            raise RuntimeError('Missing or out-of-order fixed observer')
        fields, objects = {}, {}
        commands = [cmd('info registers ' + ' '.join(REGS)), cmd('p/u phase')]
        for name in FIELDS:
            value = gdb.parse_and_eval(name)
            ctype = value.type.strip_typedefs()
            address = int(value.address)
            fields[name] = int(value)
            objects[name] = {'address': hx(address), 'width': ctype.sizeof * 8,
                             'type': str(ctype), 'signed': int(gdb.Value(-1).cast(ctype)) < 0,
                             'bytes': list(bytes(gdb.selected_inferior().read_memory(address, ctype.sizeof)))}
            commands.extend((cmd(('p/d ' if objects[name]['signed'] else 'p/u ') + name), cmd('p/x &' + name),
                             cmd('x/%dbx &' % ctype.sizeof + name)))
        source, callsite = frame.find_sal(), frame.older().find_sal()
        samples.append({'index': index, 'event': event, 'phase': index + 1,
                        'pid': gdb.selected_inferior().pid, 'fields': fields, 'objects': objects,
                        'registers': {key: hx(gdb.parse_and_eval('$' + key)) for key in REGS},
                        'commands': commands,
                        'source': {'file': source.symtab.filename, 'line': source.line, 'function': frame.name()},
                        'callsite': {'file': callsite.symtab.filename, 'line': callsite.line, 'function': 'main'}})
    cmd('delete breakpoints')
    gdb.events.exited.connect(lambda event: EXITS.append(event.exit_code))
    cmd('continue')
    if EXITS != [0]:
        raise RuntimeError('Expected normal exit0, got ' + repr(EXITS))
    with open('.trace-build/integer-bits/stdout.txt', encoding='utf8') as f:
        stdout = f.read()
    with open('.trace-build/integer-bits/raw.json', 'w', encoding='utf8') as f:
        json.dump({'samples': samples, 'types': types, 'exitCode': 0, 'stdout': stdout,
                   'gdb': gdb.VERSION, 'architecture': architecture,
                   'randomization': cmd('show disable-randomization')['output'],
                   'gdbText': '\n\n'.join(LOG)}, f, indent=2)
except Exception:
    gdb.write(traceback.format_exc(), gdb.STDERR)
    gdb.execute('quit 1')
