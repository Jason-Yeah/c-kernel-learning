"""Reviewed control-flow.c only. Never invent kernel PCs or read PROT_NONE data."""
import gdb
import json
import traceback

LOG, EXITS, SIGNALS = [], [], []
REGS = ('rip', 'rsp', 'rbp', 'rax', 'rdi', 'rsi', 'rdx', 'rcx', 'r11', 'eflags', 'cs')
FIELDS = ('input_a', 'input_b', 'observed_sum', 'observed_pid', 'syscall_number',
          'read_syscall_number', 'page_size', 'fault_read_complete', 'fault_value')


def cmd(text):
    output = gdb.execute(text, to_string=True).strip()
    LOG.append('(gdb) ' + text + '\n' + output)
    return {'command': text, 'output': output}


def hx(value, width=64):
    return '0x%X' % (int(value) & ((1 << width) - 1))


def memory(address, count):
    return list(bytes(gdb.selected_inferior().read_memory(address, count)))


def capture(event, samples):
    frame = gdb.selected_frame()
    pc = int(gdb.parse_and_eval('$rip'))
    ins = frame.architecture().disassemble(pc, count=1)[0]
    sal = frame.find_sal()
    caller = frame.older()
    older = caller.find_sal() if caller else None
    s = {'index': len(samples), 'event': event, 'pid': gdb.selected_inferior().pid,
         'registers': {r: hx(gdb.parse_and_eval('$'+r), 16 if r == 'cs' else 32 if r == 'eflags' else 64) for r in REGS},
         'fields': {name: int(gdb.parse_and_eval(name)) for name in FIELDS},
         'guardAddress': hx(gdb.parse_and_eval('guard_page')),
         'source': {'file': sal.symtab.filename if sal.symtab else None, 'line': sal.line, 'function': frame.name()},
         'callsite': {'file': older.symtab.filename if older and older.symtab else None, 'line': older.line if older else 0, 'function': caller.name() if caller else None},
         'instruction': {'address': hx(pc), 'length': ins['length'], 'asm': ins['asm'], 'bytes': memory(pc, ins['length'])}}
    s['commands'] = [cmd(c) for c in ('info registers '+' '.join(REGS), 'x/i $rip', 'x/%dbx $rip' % ins['length'])]
    s['commands'] += [cmd('p/d '+name) for name in FIELDS]
    s['commands'].append(cmd('p/x guard_page'))
    if event == 'function-entry':
        rsp = int(gdb.parse_and_eval('$rsp'))
        s['returnAddress'] = hx(int.from_bytes(bytes(memory(rsp, 8)), 'little'))
        s['commands'].append(cmd('x/gx $rsp'))
    if event == 'fault-ready':
        s['guardByte'] = memory(int(s['guardAddress'], 16), 1)[0]
        s['commands'].append(cmd('x/1bx guard_page'))
    if event in ('fault-before', 'fault-signal'):
        path = '/proc/%d/maps' % s['pid']
        with open(path, encoding='utf8') as f:
            s['maps'] = {'path': path, 'content': f.read()}
        s['commands'].append(cmd('info proc mappings'))
    if event == 'fault-signal':
        s['signal'] = {'name': 'SIGSEGV', 'number': int(gdb.parse_and_eval('$_siginfo.si_signo')),
                       'code': int(gdb.parse_and_eval('$_siginfo.si_code')),
                       'address': hx(gdb.parse_and_eval('$_siginfo')['_sifields']['_sigfault']['si_addr'])}
        s['commands'].extend(cmd(c) for c in ('p $_siginfo.si_signo', 'p $_siginfo.si_code', 'p/x $_siginfo.si_addr'))
    samples.append(s)
    return s


try:
    for c in ('set pagination off', 'set confirm off', 'set style enabled off',
              'set disassembly-flavor att', 'set print frame-arguments none', 'set disable-randomization off',
              'handle SIGSEGV stop print pass', 'file .trace-build/control-flow/program'):
        cmd(c)
    gdb.events.exited.connect(lambda e: EXITS.append(getattr(e, 'exit_code', None)))
    gdb.events.stop.connect(lambda e: SIGNALS.append(e.stop_signal) if isinstance(e, gdb.SignalEvent) else None)
    runs = {}
    for mode in ('inspect', 'fault'):
        cmd('start '+mode+' > .trace-build/control-flow/'+mode+'-stdout.txt')
        architecture = gdb.selected_frame().architecture().name()
        bp = gdb.Breakpoint('checkpoint', internal=True)
        cmd('continue')
        if gdb.selected_frame().name() != 'checkpoint':
            raise RuntimeError('Expected initialized observer breakpoint')
        samples = []
        capture('ready' if mode == 'inspect' else 'fault-ready', samples)
        if mode == 'inspect':
            gdb.Breakpoint('*local_add', temporary=True, internal=True)
            cmd('continue')
            capture('function-entry', samples)
            cmd('finish')
            capture('function-return', samples)
            cmd('continue')
            capture('sum-stored', samples)
            gdb.Breakpoint('*ec_syscall', temporary=True, internal=True)
            cmd('continue')
            capture('syscall-before', samples)
            cmd('si')
            capture('syscall-after', samples)
            cmd('continue')
            capture('pid-stored', samples)
        else:
            gdb.Breakpoint('*ec_fault', temporary=True, internal=True)
            cmd('continue')
            capture('fault-before', samples)
            signals_before = len(SIGNALS)
            cmd('si')
            if len(SIGNALS) != signals_before+1 or SIGNALS[-1] != 'SIGSEGV':
                raise RuntimeError('Expected deliberate own-page fault')
            capture('fault-signal', samples)
        cmd('delete breakpoints')
        before = len(EXITS)
        completion = cmd('continue')
        if len(EXITS) != before+1:
            raise RuntimeError('Missing inferior exit event')
        if mode == 'inspect':
            if EXITS[-1] != 0:
                raise RuntimeError('Inspect did not normally exit0')
            outcome = {'kind': 'exit', 'code': 0}
        else:
            if EXITS[-1] is not None or SIGNALS[-1] != 'SIGSEGV':
                raise RuntimeError('Fault signal cleanup mislabeled as normal exit')
            outcome = {'kind': 'signal', 'signal': 'SIGSEGV'}
        with open('.trace-build/control-flow/'+mode+'-stdout.txt', encoding='utf8') as f:
            stdout = f.read()
        runs[mode] = {'samples': samples, 'outcome': outcome, 'stdout': stdout}
    with open('.trace-build/control-flow/raw.json', 'w', encoding='utf8') as f:
        json.dump({'runs': runs, 'architecture': architecture, 'gdb': gdb.VERSION,
                   'randomization': cmd('show disable-randomization')['output'],
                   'gdbText': '\n\n'.join(LOG)}, f, indent=2)
except Exception:
    gdb.write(traceback.format_exc(), gdb.STDERR)
    gdb.execute('quit 1')
