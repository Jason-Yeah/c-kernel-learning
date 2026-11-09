// Minimal checked ELF64/x86-64 reader. All numbers come from actual file bytes.
const hx = (n) => `0x${BigInt(n).toString(16).toUpperCase()}`;
export function parseELF(bytes) {
  const u = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
  const v = new DataView(u.buffer, u.byteOffset, u.byteLength);
  const fail = (text) => {
    throw new Error(`ELF evidence: ${text}`);
  };
  const bounds = (off, size) => {
    if (!Number.isSafeInteger(off) || off < 0 || off + size > u.length)
      fail("truncated or invalid range");
  };
  const n16 = (off) => {
    bounds(off, 2);
    return v.getUint16(off, true);
  };
  const n32 = (off) => {
    bounds(off, 4);
    return v.getUint32(off, true);
  };
  const n64 = (off) => {
    bounds(off, 8);
    return v.getBigUint64(off, true);
  };
  const number = (n) => {
    const result = Number(n);
    if (!Number.isSafeInteger(result)) fail("oversized file offset");
    return result;
  };
  bounds(0, 64);
  if (
    u[0] !== 0x7f ||
    String.fromCharCode(...u.slice(1, 4)) !== "ELF" ||
    u[4] !== 2 ||
    u[5] !== 1 ||
    n16(18) !== 62
  )
    fail("requires little-endian ELF64 x86-64");
  const text = (base, index, limit) => {
    bounds(base, limit);
    if (index >= limit) fail("invalid string-table index");
    let end = base + index;
    while (end < base + limit && u[end]) end++;
    return new TextDecoder().decode(u.slice(base + index, end));
  };
  const phoff = number(n64(32)),
    shoff = number(n64(40)),
    phsize = n16(54),
    phnum = n16(56),
    shsize = n16(58),
    shnum = n16(60),
    shstr = n16(62);
  if ((phnum && phsize !== 56) || shsize !== 64 || !shnum || shstr >= shnum)
    fail("unsupported ELF table layout");
  const sections = [];
  for (let i = 0; i < shnum; i++) {
    const p = shoff + i * shsize;
    bounds(p, 64);
    sections.push({
      index: i,
      nameOffset: n32(p),
      type: n32(p + 4),
      flags: hx(n64(p + 8)),
      address: hx(n64(p + 16)),
      offset: number(n64(p + 24)),
      size: number(n64(p + 32)),
      link: n32(p + 40),
      info: n32(p + 44),
      alignment: number(n64(p + 48)),
      entrySize: number(n64(p + 56)),
    });
  }
  const names = sections[shstr];
  for (const section of sections) {
    section.name = text(names.offset, section.nameOffset, names.size);
    delete section.nameOffset;
    if (section.type !== 8) bounds(section.offset, section.size);
    section.fileBytes = section.type === 8 ? 0 : section.size;
  }
  const programs = [];
  for (let i = 0; i < phnum; i++) {
    const p = phoff + i * phsize;
    bounds(p, 56);
    const entry = {
      index: i,
      type: n32(p),
      flags: n32(p + 4),
      offset: number(n64(p + 8)),
      vaddr: hx(n64(p + 16)),
      paddr: hx(n64(p + 24)),
      fileSize: number(n64(p + 32)),
      memorySize: number(n64(p + 40)),
      alignment: number(n64(p + 48)),
    };
    bounds(entry.offset, entry.fileSize);
    if (entry.type === 3)
      entry.interpreter = new TextDecoder()
        .decode(u.slice(entry.offset, entry.offset + entry.fileSize))
        .replace(/\0.*$/s, "");
    programs.push(entry);
  }
  const symbols = [],
    tables = new Map();
  for (const section of sections.filter((s) => s.type === 2 || s.type === 11)) {
    if (
      section.entrySize !== 24 ||
      section.size % 24 ||
      !sections[section.link]
    )
      fail("invalid symbol table");
    const strs = sections[section.link],
      table = [];
    for (let i = 0; i < section.size / 24; i++) {
      const p = section.offset + i * 24,
        info = u[p + 4],
        shndx = n16(p + 6);
      const symbol = {
        index: i,
        table: section.name,
        name: text(strs.offset, n32(p), strs.size),
        binding: info >> 4,
        type: info & 15,
        sectionIndex: shndx,
        section:
          shndx === 0 ? "UND" : sections[shndx]?.name || `INDEX_${shndx}`,
        value: hx(n64(p + 8)),
        size: number(n64(p + 16)),
      };
      table.push(symbol);
      symbols.push(symbol);
    }
    tables.set(section.index, table);
  }
  const relocations = [];
  for (const section of sections.filter((s) => s.type === 4)) {
    if (
      section.entrySize !== 24 ||
      section.size % 24 ||
      !tables.has(section.link)
    )
      fail("invalid RELA table");
    for (let i = 0; i < section.size / 24; i++) {
      const p = section.offset + i * 24,
        info = n64(p + 8),
        type = Number(info & 0xffffffffn),
        symbol = tables.get(section.link)[Number(info >> 32n)];
      if (!symbol) fail("invalid relocation symbol");
      relocations.push({
        section: section.name,
        targetSection: sections[section.info]?.name || null,
        offset: hx(n64(p)),
        type,
        typeName:
          {
            1: "R_X86_64_64",
            2: "R_X86_64_PC32",
            4: "R_X86_64_PLT32",
            9: "R_X86_64_GOTPCREL",
          }[type] || `TYPE_${type}`,
        symbol: symbol.name,
        addend: v.getBigInt64(p + 16, true).toString(),
      });
    }
  }
  return {
    header: {
      type: n16(16),
      machine: n16(18),
      entry: hx(n64(24)),
      size: n16(52),
      phoff,
      phnum,
      phsize,
      shoff,
      shnum,
      shsize,
      shstr,
    },
    sections,
    programs,
    symbols,
    relocations,
  };
}
export function elfSymbol(elf, name) {
  const value = elf.symbols.find(
    (s) => s.table === ".symtab" && s.name === name && s.section !== "UND",
  );
  if (!value) throw new Error(`ELF definition missing: ${name}`);
  return value;
}
export function bytesAtVA(bytes, elf, address, count) {
  const at = BigInt(address),
    segment = elf.programs.find(
      (p) =>
        p.type === 1 &&
        at >= BigInt(p.vaddr) &&
        at + BigInt(count) <= BigInt(p.vaddr) + BigInt(p.fileSize),
    );
  if (!segment) throw new Error(`No file-backed PT_LOAD for ${address}`);
  return [
    ...bytes.slice(
      segment.offset + Number(at - BigInt(segment.vaddr)),
      segment.offset + Number(at - BigInt(segment.vaddr)) + count,
    ),
  ];
}
