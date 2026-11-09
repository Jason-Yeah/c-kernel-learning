"""Reviewed chapter0 experiment only; invoked by capture-os-basics.mjs."""
import gdb
import json
import os
import struct
import traceback

ROOT = os.getcwd()
WORK = os.path.join(ROOT, '.trace-build/os-basics')
CASE = os.environ['OS_RUN']
REGS = ('rax','rbx','rcx','rdx','rsi','rdi','rbp','rsp','r8','r9','r10','r11','r12','r13','r14','r15','rip','eflags')
LOG, SAMPLES, EXITS = [], [], []
LAST = None
with open(os.path.join(WORK,'elf.json'), encoding='utf8') as f:
    ELF = json.load(f)


def hx(value):
    return '0x%X' % (int(value) & ((1 << 64) - 1))


def ev(expr):
    return int(gdb.parse_and_eval(expr))


def cmd(text):
    output = gdb.execute(text, to_string=True).strip()
    LOG.append('(gdb) ' + text + '\n' + output)
    return {'command': text, 'output': output}


def stop(event):
    global LAST
    LAST = event


def exit_event(event):
    EXITS.append({'inferior':event.inferior.num,'code':getattr(event,'exit_code',None)})


def proc_maps(pid):
    with open('/proc/%d/maps' % pid,encoding='utf8') as f:
        return f.read().splitlines()


def auxv(pid):
    with open('/proc/%d/auxv' % pid,'rb') as f:
        data=f.read()
    return {str(a):hx(b) for a,b in struct.iter_unpack('<QQ',data) if a}


def location(frame):
    sal=frame.find_sal()
    return {'file':sal.symtab.filename if sal.symtab else None,'line':sal.line,'function':frame.name()}


def instruction():
    return gdb.selected_frame().architecture().disassemble(ev('$rip'),count=1)[0]


def sample(event,initialized=False,previous=None,with_maps=False):
    inf=gdb.selected_inferior()
    frame=gdb.selected_frame()
    item=instruction()
    regs={name:hx(ev('$'+name)) for name in REGS}
    value={'index':len(SAMPLES),'event':event,'pid':inf.pid,'inferior':inf.num,'source':location(frame),'registers':regs,
           'instruction':{'address':hx(item['addr']),'asm':item['asm'],'length':item['length'],'bytes':list(bytes(inf.read_memory(item['addr'],item['length'])))},'commands':[]}
    if previous:
        value['previousInstruction']=previous
    commands=['info registers '+ ' '.join(REGS),'x/i $rip']
    if with_maps:
        value['maps']=proc_maps(inf.pid)
        value['auxv']=auxv(inf.pid)
        commands += ['info proc mappings','info auxv']
    if event=='app-entry':
        data=bytes(inf.read_memory(ev('$rsp'),40))
        value['initialStack']=[hx(n) for n in struct.unpack('<QQQQQ',data)]
        commands+=['x/5gx $rsp']
    if initialized:
        value['phase']=ev('os_phase')
        value['role']=ev('os_role')
        value['forkResult']=ev('os_fork_result')
        value['childPid']=ev('os_child')
        value['pageSize']=ev('os_page_size')
        value['result']=ev('os_result')
        value['widthResult']=hx(ev('os_width_result'))
        value['equalResult']=ev('os_equal_result')
        value['pointerVariable']={'address':hx(ev('&os_heap')),'value':hx(ev('os_heap'))}
        objects=[]
        for name,expr,val,size,kind in (
            ('text','&main',None,1,'code'),('rodata','&os_message[0]',None,8,'rodata'),
            ('data','&os_data','os_data',4,'data'),('bss','&os_bss','os_bss',4,'bss'),
            ('heap','os_heap','*os_heap',4,'heap'),('private','os_private','*os_private',4,'private'),
            ('shared','os_shared','*os_shared',4,'shared'),('stack','os_stack','*os_stack',4,'stack')):
            address=ev(expr)
            obj={'name':name,'kind':kind,'address':hx(address),'size':size,'bytes':list(bytes(inf.read_memory(address,size)))}
            if val:
                obj['value']=ev(val)
            objects.append(obj)
        value['objects']=objects
        commands+=['p os_phase','p os_fork_result','p os_child','p/x &os_data','p os_data','p os_bss','p/x os_stack','p *os_stack','p/x os_heap','p *os_heap','p/x os_private','p *os_private','p/x os_shared','p *os_shared','p os_result']
    if event.startswith('os_add'):
        value['stackTop']=hx(int.from_bytes(bytes(inf.read_memory(ev('$rsp'),8)),'little'))
        commands+=['x/gx $rsp']
    value['commands']=[cmd(c) for c in commands]
    LOG.append('\n=== %s / %s / PID %d ===' % (CASE,event,inf.pid))
    SAMPLES.append(value)
    return value


def resume():
    global LAST
    LAST=None
    if not gdb.selected_inferior().pid:
        live=next((i for i in gdb.inferiors() if i.pid),None)
        if live:
            cmd('inferior %d'%live.num)
    cmd('continue')
    if isinstance(LAST,gdb.SignalEvent):
        raise RuntimeError('Unexpected signal: '+LAST.stop_signal)
    return LAST


def checkpoint():
    with open('experiments/os-main.c',encoding='utf8') as f:
        lines=f.readlines()
    line=next(i+1 for i,s in enumerate(lines) if 'TRACE_OS_EVENT' in s)
    return gdb.Breakpoint('experiments/os-main.c:%d'%line,internal=True)


def probe(name):
    start=ev('(void*)&'+name)
    size=next(s['size'] for s in ELF['symbols'] if s['table']=='.symtab' and s['name']==name and s['section']!='UND')
    instructions=gdb.selected_frame().architecture().disassemble(start,start+size)
    DISASSEMBLY[name]=[{'address':hx(i['addr']),'asm':i['asm'],'length':i['length']} for i in instructions]
    gdb.Breakpoint('*'+hx(start),temporary=True,internal=True)
    resume()
    sample(name+'-entry')
    for _ in range(40):
        old=instruction()
        old={'address':hx(old['addr']),'asm':old['asm'],'length':old['length']}
        cmd('si')
        if old['asm'].startswith('ret'):
            sample(name+'-returned',previous=old)
            return
        sample(name+'-step',previous=old)
    raise RuntimeError('Unbounded instruction probe')


DISASSEMBLY={}
try:
    gdb.events.stop.connect(stop)
    gdb.events.exited.connect(exit_event)
    for setting in ['set pagination off','set confirm off','set print elements 32','set disassembly-flavor att','set disable-randomization off']:
        cmd(setting)
    cmd('file '+os.path.join(WORK,'os-demo'))
    cmd('set args '+('fork' if CASE=='fork' else 'inspect'))
    if CASE=='fork':
        for setting in ['set detach-on-fork off','set follow-fork-mode parent','set schedule-multiple on']:
            cmd(setting)
        checkpoint()
        cmd('run fork > '+os.path.join(WORK,CASE+'.stdout'))
        sample('phase-'+str(ev('os_phase')),True,with_maps=True)
        for _ in range(32):
            if not any(i.pid for i in gdb.inferiors()):
                break
            event=resume()
            if isinstance(event,gdb.BreakpointEvent):
                sample('phase-'+str(ev('os_phase')),True,with_maps=True)
        else:
            raise RuntimeError('Fork capture did not terminate')
    else:
        cmd('starti inspect > '+os.path.join(WORK,CASE+'.stdout'))
        architecture=gdb.selected_frame().architecture().name()
        first=sample('interpreter-entry',with_maps=True)
        entry=int(first['auxv']['9'],16)
        gdb.Breakpoint('*'+hx(entry),temporary=True,internal=True)
        resume()
        sample('app-entry',with_maps=True)
        gdb.Breakpoint('*'+hx(ev('(void*)&main')),temporary=True,internal=True)
        resume()
        sample('main-entry',with_maps=True)
        point=checkpoint()
        resume()
        sample('layout-ready',True,with_maps=True)
        if CASE=='first':
            for function in ['os_add','os_width','os_equal']:
                probe(function)
        resume()
        sample('compute-done',True,with_maps=True)
        point.delete()
        while any(i.pid for i in gdb.inferiors()):
            resume()
    with open(os.path.join(WORK,CASE+'.stdout'),encoding='utf8') as f:
        stdout=f.read()
    result={'case':CASE,'gdb':gdb.VERSION,'architecture':locals().get('architecture','i386:x86-64'),'samples':SAMPLES,'disassembly':DISASSEMBLY,'exits':EXITS,'stdout':stdout,'commandsText':'\n\n'.join(LOG)}
    with open(os.path.join(WORK,CASE+'.json'),'w',encoding='utf8') as f:
        json.dump(result,f,ensure_ascii=False,indent=2)
except Exception:
    traceback.print_exc()
    gdb.execute('quit 1')
