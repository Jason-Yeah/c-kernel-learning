"""Fixed, reviewed local C experiments only. Run inside GDB via capture-experiments.mjs."""
import gdb
import json
import os
import re
import traceback

TOPIC = os.environ["ANIMATION_EXPERIMENT"]
CASE = os.environ["ANIMATION_CASE"]
SPECS = {
    "buffer-copy": ("experiments/buffer-copy.c", ("TRACE_BUFFER_READY", "TRACE_BUFFER_BYTE", "TRACE_BUFFER_DONE", "TRACE_BUFFER_RETURNED")),
    "stack-recursion": ("experiments/stack-recursion.c", ("TRACE_RECURSION_ENTER", "TRACE_RECURSION_LEAVE", "TRACE_RECURSION_RESULT")),
    "cache-access": ("experiments/cache-access.c", ("TRACE_CACHE_READY", "TRACE_CACHE_READ")),
    "process-lifecycle": ("experiments/process-lifecycle.c", ("TRACE_PROCESS_EVENT",)),
    "fork-memory": ("experiments/fork-memory.c", ("TRACE_FORK_MEMORY_EVENT",)),
    "vm-access": ("experiments/vm-access.c", ("TRACE_VM_BEFORE", "TRACE_VM_AFTER")),
}
SOURCE, MARKERS = SPECS[TOPIC]
BINARY = ".trace-build/" + TOPIC
STDOUT = ".trace-build/" + TOPIC + "-" + CASE + ".stdout"
OUTPUT = ".trace-build/" + TOPIC + "-" + CASE + ".json"
REGISTERS = ("rip", "rsp", "rbp", "rax", "rdi", "rsi")
LAST_STOP = None
EXITS = []
LOG = []
CONTEXT = {}


def hx(value):
    # GDB sometimes exposes a 64-bit register through a signed long type.
    return "0x" + format(int(value) & ((1 << 64) - 1), "X")


def integer(expression, default=None):
    try:
        return int(gdb.parse_and_eval(expression))
    except (gdb.error, ValueError, TypeError):
        if default is not None:
            return default
        raise


def read_bytes(address, count):
    return list(bytes(gdb.selected_inferior().read_memory(address, count)))


def word(address):
    return hx(int.from_bytes(bytes(read_bytes(address, 8)), "little"))


def command(text):
    try:
        output = gdb.execute(text, to_string=True).strip()
    except gdb.error as error:
        output = "GDB: " + str(error)
    LOG.append("(gdb) " + text + "\n" + output)
    return {"command": text, "output": output}


def source_location(frame):
    sal = frame.find_sal()
    return {"file": sal.symtab.filename if sal.symtab else None, "line": sal.line, "function": frame.name()}


def maps(pid):
    with open("/proc/%d/maps" % pid, encoding="utf-8") as handle:
        return handle.read().splitlines()


def mapping_for(lines, address):
    for line in lines:
        region = line.split()[0].split("-")
        if int(region[0], 16) <= address < int(region[1], 16):
            return line
    return None


def proc_state(pid):
    try:
        with open("/proc/%d/stat" % pid, encoding="utf-8") as handle:
            # comm may contain spaces or parentheses; state follows its final ')'.
            rest = handle.read().rsplit(")", 1)[1].split()
        return {"state": rest[0], "ppid": int(rest[1])}
    except FileNotFoundError:
        return None


def pagemap(pid, address, page_size):
    # Reading pagemap flags does NOT fault the cold user target page in.
    with open("/proc/%d/pagemap" % pid, "rb", buffering=0) as handle:
        handle.seek((address // page_size) * 8)
        value = int.from_bytes(handle.read(8), "little")
    return {"raw": hx(value), "present": bool(value & (1 << 63)), "swapped": bool(value & (1 << 62)), "exclusive": bool(value & (1 << 56)), "pfnExposed": bool(value & ((1 << 55) - 1))}


def on_stop(event):
    global LAST_STOP
    LAST_STOP = event


def on_exit(event):
    EXITS.append({"inferior": event.inferior.num, "code": getattr(event, "exit_code", None)})


def snapshot(index):
    inferior = gdb.selected_inferior()
    frame = gdb.selected_frame()
    pc = integer("$rip")
    phase = integer("trace_phase", -1)
    signal = isinstance(LAST_STOP, gdb.SignalEvent)
    event = "signal" if signal else "phase-%d" % phase
    if not signal and TOPIC == "buffer-copy" and pc == CONTEXT.get("retPC"):
        event = "ret"
    if not signal and TOPIC == "process-lifecycle" and phase == 0:
        event = "exec"
    location = source_location(frame)
    caller = frame.older() if frame.name() == "checkpoint" else None
    callsite = source_location(caller) if caller else location
    sample = {"index": index, "event": event, "phase": phase, "pid": inferior.pid, "inferior": inferior.num,
              "source": location, "callsite": callsite,
              "registers": {name: hx(integer("$" + name)) for name in REGISTERS}, "fields": {}, "commands": []}
    lines = maps(inferior.pid)
    fields = sample["fields"]
    commands = ["info registers rip rsp rbp rax", "x/i $rip"]
    if signal:
        sample["signal"] = {"name": LAST_STOP.stop_signal, "number": integer("$_siginfo.si_signo"), "code": integer("$_siginfo.si_code"), "address": hx(integer("$_siginfo._sifields._sigfault.si_addr"))}
        commands.append("p $_siginfo")
    if TOPIC == "buffer-copy":
        if "bufferAddress" not in CONTEXT:
            base = integer("&buf[0]")
            rbp = integer("$rbp")
            block = gdb.block_for_pc(pc)
            while block and not block.function:
                block = block.superblock
            instructions = frame.architecture().disassemble(block.start, block.end)
            ret = next(item["addr"] for item in instructions if item["asm"].startswith("ret"))
            CONTEXT.update({"bufferAddress": base, "rbpSlot": rbp, "returnSlot": rbp + 8, "retPC": ret, "span": rbp + 16 - base})
            if not 16 <= CONTEXT["span"] <= 96:
                raise RuntimeError("Unexpected buffer frame layout; no guessed fallback")
            gdb.Breakpoint("*" + hx(ret), internal=True)
        base = CONTEXT["bufferAddress"]
        fields.update({"bufferAddress": hx(base), "capacity": 8, "rbpSlot": hx(CONTEXT["rbpSlot"]), "returnSlot": hx(CONTEXT["returnSlot"]), "inputAddress": hx(integer("input")), "inputLength": integer("input_length"), "index": integer("copy_index"), "memory": read_bytes(base, CONTEXT["span"]), "savedRbp": word(CONTEXT["rbpSlot"]), "savedReturn": word(CONTEXT["returnSlot"]), "input": read_bytes(integer("input"), integer("input_length"))})
        commands += ["p copy_index", "p/x input", "x/%dbx %s" % (fields["inputLength"], fields["inputAddress"]), "x/%dbx %s" % (CONTEXT["span"], hx(base)), "x/gx " + hx(CONTEXT["returnSlot"])]
    elif TOPIC == "stack-recursion":
        fields.update({"stackLimit": integer("stack_limit"), "boundedDepth": integer("bounded_depth"), "stackMapping": next((line for line in lines if "[stack]" in line), None)})
        if frame.name() == "f" and not signal:
            fields.update({"depth": integer("depth"), "blockAddress": hx(integer("&block[0]")), "blockBytes": integer("sizeof block"), "savedReturn": word(integer("$rbp") + 8)})
            if phase == 2:
                fields["childResult"] = integer("child")
            commands += ["p depth", "p/x &block[0]", "p sizeof block", "p stack_limit"]
        if signal:
            frames, current = [], frame
            while current and len(frames) < 256:
                if current.name() == "f":
                    frames.append({"function": "f", "pc": hx(current.pc())})
                try:
                    current = current.older()
                except gdb.error:
                    break
            fields["recursiveFrames"] = frames
            commands.append("bt 8")
        if phase == 3:
            fields["result"] = integer("observed_sum")
            commands.append("p observed_sum")
    elif TOPIC == "cache-access":
        fields["base"] = hx(integer("&memory[0]"))
        if phase == 1:
            fields["memory"] = read_bytes(integer("&memory[0]"), 256)
            fields["offsets"] = [integer("offsets[%d]" % i) for i in range(8)]
            commands += ["p/x &memory[0]", "x/16bx &memory[0]"]
        else:
            fields.update({"step": integer("step"), "offset": integer("offset"), "target": hx(integer("target")), "value": integer("value"), "sink": integer("sink"), "byte": read_bytes(integer("target"), 1)[0], "al": integer("$al") & 0xFF})
            commands += ["p step", "p/x target", "x/1bx target", "p/x value", "p/x $al"]
    elif TOPIC == "process-lifecycle":
        fields.update({"role": integer("role"), "forkResult": integer("fork_result"), "childPid": integer("child_id"), "waitResult": integer("wait_result"), "waitStatus": integer("wait_status")})
        if fields["role"] == 0 and fields["childPid"] > 0:
            fields["childState"] = proc_state(fields["childPid"])
        commands += ["info inferiors", "p fork_result", "p child_id", "p wait_result", "p/x wait_status"]
        if fields.get("childState"):
            LOG.append("/proc/%d/stat state = %s" % (fields["childPid"], fields["childState"]["state"]))
    elif TOPIC == "fork-memory":
        private = integer("private_page")
        shared = integer("shared_page")
        fields.update({"role": integer("role"), "forkResult": integer("fork_result"), "childPid": integer("child_id"), "pageSize": integer("page_size"), "privateAddress": hx(private), "sharedAddress": hx(shared), "privateValue": integer("*private_page"), "sharedValue": integer("*shared_page"), "privateMapping": mapping_for(lines, private), "sharedMapping": mapping_for(lines, shared), "globalData": integer("global_data"), "globalBss": integer("global_bss"), "localValue": integer("*stack_local"), "heapValue": integer("*heap_object"), "localAddress": hx(integer("stack_local")), "heapAddress": hx(integer("heap_object"))})
        commands += ["info inferiors", "p private_page", "p *private_page", "p shared_page", "p *shared_page", "p global_data", "p global_bss", "p *stack_local", "p *heap_object"]
    elif TOPIC == "vm-access":
        address = integer("target")
        page_size = integer("page_size")
        fields.update({"target": hx(address), "pageSize": page_size, "value": integer("value"), "beforeFaults": integer("before_faults"), "afterFaults": integer("after_faults"), "mapping": mapping_for(lines, address), "page": pagemap(inferior.pid, address, page_size)})
        commands += ["p/x target", "p page_size"]
        # No x/..., dereference or read_memory on target at phase 1, even for the cold case.
        if phase == 2 and not signal:
            fields["byte"] = read_bytes(address, 1)[0]
            commands += ["x/1bx target", "p/x value", "p after_faults - before_faults"]
        elif signal and CASE == "readonly":
            fields["byte"] = read_bytes(address, 1)[0]
            commands.append("x/1bx target")
        LOG.append("/proc pagemap flags (no PFN inferred): " + json.dumps(fields["page"]))
        LOG.append("/proc maps target: " + str(fields["mapping"]))
    LOG.append("\n=== %s / %s / PID %d / sample %d ===" % (CASE, event, inferior.pid, index))
    sample["commands"] = [command(text) for text in commands]
    return sample


def run():
    for text in ("set pagination off", "set confirm off", "set print elements 0", "set style enabled off", "set disassembly-flavor att", "set disable-randomization off"):
        gdb.execute(text, to_string=True)
    if TOPIC in ("process-lifecycle", "fork-memory"):
        for text in ("set detach-on-fork off", "set follow-fork-mode parent", "set schedule-multiple on"):
            gdb.execute(text, to_string=True)
    gdb.execute("file " + BINARY, to_string=True)
    if TOPIC == "process-lifecycle":
        gdb.execute("catch exec", to_string=True)
    with open(SOURCE, encoding="utf-8") as handle:
        source = handle.read().splitlines()
    for marker in MARKERS:
        line = next(i + 1 for i, text in enumerate(source) if re.search(r"\b" + re.escape(marker) + r"\b", text))
        gdb.Breakpoint(SOURCE + ":" + str(line), internal=True)
    gdb.events.stop.connect(on_stop)
    gdb.events.exited.connect(on_exit)
    samples = []
    first = True
    architecture = None
    outcome = "exit"
    for index in range(500):
        global LAST_STOP
        LAST_STOP = None
        exits_before = len(EXITS)
        if first:
            args = "" if CASE == "default" else " " + CASE
            command("run" + args + " > " + STDOUT)
            first = False
        else:
            active = [item for item in gdb.inferiors() if item.pid and item.threads()]
            if not active:
                break
            if not gdb.selected_inferior().pid or not gdb.selected_inferior().threads():
                gdb.execute("inferior %d" % active[0].num, to_string=True)
            command("continue")
        if LAST_STOP is None:
            if not any(item.pid and item.threads() for item in gdb.inferiors()):
                break
            if len(EXITS) > exits_before:
                # GDB stops on an inferior exit even while its parent is still live.
                # Select that surviving inferior on the next iteration and resume it.
                continue
            raise RuntimeError("Inferior remained live without an explained stop")
        if architecture is None:
            architecture = gdb.selected_frame().architecture().name()
        samples.append(snapshot(len(samples)))
        if isinstance(LAST_STOP, gdb.SignalEvent):
            outcome = "signal"
            # Keep the observed signal snapshot; cleanup kill is NOT reported as normal exit.
            command("kill")
            break
    else:
        raise RuntimeError("Unexpectedly long run; bounded collector stopped")
    if not samples:
        raise RuntimeError("No actual breakpoint snapshots")
    with open(STDOUT, encoding="utf-8", errors="replace") as handle:
        stdout = handle.read()
    result = {"case": CASE, "architecture": architecture, "outcome": outcome, "exits": EXITS, "samples": samples, "stdout": stdout, "gdbVersion": gdb.VERSION, "randomization": gdb.execute("show disable-randomization", to_string=True).strip(), "gdbText": "\n\n".join(LOG) + "\n"}
    with open(OUTPUT, "w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=2, ensure_ascii=False)
    print("CAPTURE_OK: %s/%s: %d snapshots, %s" % (TOPIC, CASE, len(samples), outcome))


try:
    run()
except BaseException:
    gdb.write(traceback.format_exc(), gdb.STDERR)
    gdb.execute("quit 1")
