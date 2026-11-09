"""Reviewed instruction-cycle.c only; actual architectural single-step states."""
import gdb
import json
import traceback

REGS = ('rip', 'rsp', 'rbp', 'rax', 'rdx', 'eax', 'edx', 'eflags')
LOG = []
EXITS = []


def hx(value, bits=64):
    return '0x%X' % (int(value) & ((1 << bits) - 1))


def cmd(text):
    output = gdb.execute(text, to_string=True).strip()
    LOG.append('(gdb) ' + text + '\n' + output)
    return {'command': text, 'output': output}


def memory(address, count):
    return list(bytes(gdb.selected_inferior().read_memory(address, count)))


def capture(index, input_address, output_address):
    frame = gdb.selected_frame()
    regs = {name: hx(gdb.parse_and_eval('$' + name), 32 if name in ('eax', 'edx', 'eflags') else 64) for name in REGS}
    pc = int(regs['rip'], 16)
    ins = frame.architecture().disassemble(pc, count=1)[0]
    sal = frame.find_sal()
    words = memory(input_address, 8)
    output = memory(output_address, 4)
    sample = {'index': index, 'pid': gdb.selected_inferior().pid, 'registers': regs,
              'source': {'file': sal.symtab.filename if sal.symtab else None, 'line': sal.line, 'function': frame.name()},
              'instruction': {'address': hx(pc), 'length': ins['length'], 'asm': ins['asm'], 'bytes': memory(pc, ins['length'])},
              'inputs': {'address': hx(input_address), 'bytes': words, 'values': [int.from_bytes(bytes(words[i:i+4]), 'little', signed=True) for i in (0, 4)]},
              'output': {'address': hx(output_address), 'bytes': output, 'value': int.from_bytes(bytes(output), 'little', signed=True)}}
    sample['commands'] = [cmd(c) for c in (
        'info registers ' + ' '.join(REGS), 'x/i $rip', 'x/%dbx $rip' % ins['length'],
        'x/8bx inputs', 'x/2dw inputs', 'x/4bx &observed', 'p observed')]
    LOG.append('\n=== sample %d / %s ===' % (index, hx(pc)))
    return sample


try:
    for setting in ('set pagination off', 'set confirm off', 'set style enabled off', 'set disassembly-flavor att', 'set disable-randomization off'):
        cmd(setting)
    cmd('file .trace-build/instruction-cycle/program')
    cmd('start > .trace-build/instruction-cycle/stdout.txt')
    architecture = gdb.selected_frame().architecture().name()
    start = int(gdb.parse_and_eval('(void*)&calculate'))
    block = gdb.block_for_pc(start)
    while block and not block.function:
        block = block.superblock
    if not block:
        raise RuntimeError('No actual DWARF function extent')
    listing = gdb.selected_frame().architecture().disassemble(start, block.end)
    function = {'address': hx(start), 'size': block.end - start,
                'bytes': memory(start, block.end-start),
                'instructions': [{'address': hx(i['addr']), 'length': i['length'], 'asm': i['asm'], 'bytes': memory(i['addr'], i['length'])} for i in listing]}
    gdb.Breakpoint('*'+hx(start), temporary=True, internal=True)
    cmd('continue')
    input_address = int(gdb.parse_and_eval('&inputs[0]'))
    output_address = int(gdb.parse_and_eval('&observed'))
    rsp = int(gdb.parse_and_eval('$rsp'))
    return_address = hx(int.from_bytes(bytes(memory(rsp,8)), 'little'))
    cmd('x/gx $rsp')
    samples = []
    for index in range(100):
        sample = capture(index, input_address, output_address)
        samples.append(sample)
        if sample['source']['function'] != 'calculate':
            break
        cmd('si')
    else:
        raise RuntimeError('Instruction bound exceeded; no guessed fallback')
    gdb.events.exited.connect(lambda e: EXITS.append(e.exit_code))
    completion = cmd('continue')
    if EXITS != [0]:
        raise RuntimeError('Unexpected completion: '+repr(EXITS))
    with open('.trace-build/instruction-cycle/stdout.txt', encoding='utf8') as f:
        stdout = f.read()
    result = {'architecture': architecture, 'gdb': gdb.VERSION,
              'randomization': cmd('show disable-randomization')['output'],
              'function': function, 'returnAddress': return_address,
              'samples': samples, 'exitCode': 0, 'stdout': stdout, 'gdbText': '\n\n'.join(LOG)}
    with open('.trace-build/instruction-cycle/raw.json', 'w', encoding='utf8') as f:
        json.dump(result, f, indent=2, ensure_ascii=False)
except Exception:
    gdb.write(traceback.format_exc(), gdb.STDERR)
    gdb.execute('quit 1')
