"""Run only inside GDB: captures our safe, fixed main/caller/add experiment."""
import gdb
import json
import os
import traceback

ROOT = os.getcwd()
SOURCE = "experiments/stack-call.c"
OUTPUT = ".trace-build/stack-raw.json"
REGISTERS = ("rip", "rsp", "rbp", "rdi", "rsi", "rax")
VARIABLES = {"main": ("result",), "caller": ("n", "local", "result"), "add": ("a", "b", "sum")}


def hx(value):
    return "0x" + format(int(value), "X")


def word(address):
    data = bytes(gdb.selected_inferior().read_memory(address, 8))
    return {"address": hx(address), "value": hx(int.from_bytes(data, "little")), "bytes": data.hex(" ").upper()}


def variable(frame, name):
    try:
        value = frame.read_var(name)
        return {"value": int(value), "address": hx(value.address), "type": str(value.type)}
    except (gdb.error, ValueError, TypeError):
        return None


def capture(index):
    regs = {name: hx(gdb.parse_and_eval("$" + name)) for name in REGISTERS}
    pc = int(regs["rip"], 16)
    instruction = gdb.selected_frame().architecture().disassemble(pc, count=1)[0]
    frames = []
    frame = gdb.newest_frame()
    while frame and len(frames) < 5:
        name = frame.name()
        if name in VARIABLES:
            frames.append({"function": name, "variables": {key: variable(frame, key) for key in VARIABLES[name]}})
        frame = frame.older()
    sal = gdb.find_pc_line(pc)
    return {
        "index": index, "pc": hx(pc), "registers": regs,
        "instruction": instruction["asm"],
        "source": {"file": sal.symtab.filename if sal.symtab else None, "line": sal.line},
        "stack": [word(int(regs["rsp"], 16) + 8 * i) for i in range(8)],
        "frames": frames,
    }


def function_info(name, source_lines):
    start = int(gdb.parse_and_eval("(void *)" + name))
    block = gdb.block_for_pc(start)
    while block and not block.function:
        block = block.superblock
    if not block:
        raise RuntimeError("No DWARF function block for " + name)
    instructions = gdb.selected_frame().architecture().disassemble(start, block.end)
    decl = next(i for i, text in enumerate(source_lines) if ("int " + name + "(") in text)
    end = next(i for i in range(decl + 1, len(source_lines)) if source_lines[i].strip() == "}")
    return {
        "address": hx(start),
        "source": [{"line": i + 1, "text": source_lines[i]} for i in range(decl, end + 1)],
        "instructions": [{"address": hx(item["addr"]), "length": item["length"], "asm": item["asm"]} for item in instructions if item["addr"] < block.end],
    }


def line_address(lines, marker):
    line = next(i + 1 for i, text in enumerate(lines) if marker in text)
    _, entries = gdb.decode_line(SOURCE + ":" + str(line))
    if not entries:
        raise RuntimeError("No debug location for " + marker)
    return entries[0].pc


def call_instruction(info, target):
    return next(i for i in info["instructions"] if i["asm"].startswith("call") and ("<" + target + ">") in i["asm"])


def run():
    for command in ("set pagination off", "set confirm off", "set print elements 0", "set disassembly-flavor att", "set style enabled off", "set disable-randomization off"):
        gdb.execute(command, to_string=True)
    gdb.execute("file .trace-build/stack-call", to_string=True)
    # Start first: PIE symbols must be resolved after relocation, not guessed ELF offsets.
    start_log = gdb.execute("start", to_string=True)
    architecture = gdb.selected_frame().architecture().name()
    with open(SOURCE, encoding="utf-8") as handle:
        lines = handle.read().splitlines()
    functions = {name: function_info(name, lines) for name in ("main", "caller", "add")}
    main_call = call_instruction(functions["main"], "caller")
    add_call = call_instruction(functions["caller"], "add")
    main_after = int(main_call["address"], 16) + main_call["length"]
    caller_after = int(add_call["address"], 16) + add_call["length"]
    main_store = next(i for i in functions["main"]["instructions"] if int(i["address"], 16) == main_after)
    if not main_store["asm"].startswith("mov") or "%eax" not in main_store["asm"]:
        raise RuntimeError("Compiler produced a different main result store; update collector explicitly")
    main_stored = main_after + main_store["length"]
    targets = {
        "main_before_call": int(main_call["address"], 16),
        "caller_entry": int(functions["caller"]["address"], 16),
        "caller_frame_ready": line_address(lines, "TRACE_CALL_ADD"),
        "caller_before_add": int(add_call["address"], 16),
        "add_entry": int(functions["add"]["address"], 16),
        "add_frame_ready": line_address(lines, "int sum = a + b;"),
        "add_result_ready": line_address(lines, "TRACE_ADD_RESULT"),
        "add_ret": int(next(i["address"] for i in functions["add"]["instructions"] if i["asm"].startswith("ret")), 16),
        "caller_after_add": caller_after,
        "caller_ret": int(next(i["address"] for i in functions["caller"]["instructions"] if i["asm"].startswith("ret")), 16),
        "main_after_call": main_after,
        "main_result_stored": main_stored,
    }
    gdb.Breakpoint("*" + hx(targets["main_before_call"]), temporary=True, internal=True)
    gdb.execute("continue", to_string=True)
    samples, events, raw = [], {}, ["$ gdb -nx -q -batch -x scripts/gdb-stack.py", gdb.execute("show disable-randomization", to_string=True).strip(), start_log.strip()]
    for index in range(160):
        sample = capture(index)
        samples.append(sample)
        matches = [key for key, value in targets.items() if value == int(sample["pc"], 16)]
        for key in matches:
            events[key] = index
            raw.append("\n=== " + key + " / sample " + str(index) + " ===")
        if matches:
            raw.extend([gdb.execute("info registers rip rsp rbp rdi rsi rax", to_string=True).strip(), gdb.execute("x/8gx $rsp", to_string=True).strip(), gdb.execute("x/i $rip", to_string=True).strip()])
        if int(sample["pc"], 16) == main_stored:
            break
        gdb.execute("stepi", to_string=True)
    missing = set(targets) - set(events)
    if missing:
        raise RuntimeError("Required events not observed: " + repr(sorted(missing)))
    exits = []
    gdb.events.exited.connect(lambda event: exits.append(event.exit_code))
    completion = gdb.execute("continue", to_string=True)
    if exits != [0]:
        raise RuntimeError("Experiment did not exit successfully: " + repr(exits))
    raw.append(completion.strip())
    result = {"gdbVersion": gdb.VERSION, "architecture": architecture, "endian": gdb.execute("show endian", to_string=True).strip(), "randomization": gdb.execute("show disable-randomization", to_string=True).strip(), "functions": functions, "events": events, "samples": samples, "exitCode": 0, "gdbText": "\n".join(raw) + "\n"}
    with open(OUTPUT, "w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=2, ensure_ascii=False)
    print("CAPTURE_OK: " + str(len(samples)) + " instruction stops, " + str(len(events)) + " named events")


try:
    run()
except BaseException:
    gdb.write(traceback.format_exc(), gdb.STDERR)
    gdb.execute("quit 1")
