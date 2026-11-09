// Small static renderer for our controlled chapter manuscript, not arbitrary HTML.
const escape = (text) =>
  String(text).replace(
    /[&<>"']/g,
    (c) =>
      ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[
        c
      ],
  );
function inline(text) {
  const codes = [];
  let value = text.replace(
    /`([^`]+)`/g,
    (_, code) => `\uE000${codes.push(code) - 1}\uE001`,
  );
  value = escape(value)
    .replace(/\*\*(.+?)\*\*/g, "<strong>$1</strong>")
    .replace(/\[([^\]]+)\]\(([^)]+)\)/g, (_, label, href) =>
      /^(?:https?:|[.\/#]|[a-zA-Z0-9_-]+\.)/.test(href)
        ? `<a href="${href}">${label}</a>`
        : label,
    );
  return value.replace(
    /\uE000(\d+)\uE001/g,
    (_, i) => `<code>${escape(codes[Number(i)])}</code>`,
  );
}
export function renderReading(markdown) {
  const lines = markdown.split("\n"),
    result = [];
  for (let i = 0; i < lines.length; ) {
    const line = lines[i];
    if (!line.trim()) {
      i++;
      continue;
    }
    if (line.startsWith("```")) {
      const block = [];
      i++;
      while (i < lines.length && !lines[i].startsWith("```"))
        block.push(lines[i++]);
      i++;
      result.push(`<pre><code>${escape(block.join("\n"))}</code></pre>`);
      continue;
    }
    const heading = line.match(/^(#{1,4}) (.+)$/);
    if (heading) {
      const level = heading[1].length;
      result.push(`<h${level}>${inline(heading[2])}</h${level}>`);
      i++;
      continue;
    }
    if (line.startsWith("|")) {
      const rows = [];
      while (i < lines.length && lines[i].startsWith("|"))
        rows.push(
          lines[i++]
            .split("|")
            .slice(1, -1)
            .map((s) => s.trim()),
        );
      const [head, , ...body] = rows;
      result.push(
        `<div class="reading-table"><table><thead><tr>${head.map((s) => `<th>${inline(s)}</th>`).join("")}</tr></thead><tbody>${body.map((row) => `<tr>${row.map((s) => `<td>${inline(s)}</td>`).join("")}</tr>`).join("")}</tbody></table></div>`,
      );
      continue;
    }
    if (line.startsWith("- ")) {
      const list = [];
      while (i < lines.length && lines[i].startsWith("- "))
        list.push(lines[i++].slice(2));
      result.push(
        `<ul>${list.map((s) => `<li>${inline(s)}</li>`).join("")}</ul>`,
      );
      continue;
    }
    const p = [line];
    i++;
    while (
      i < lines.length &&
      lines[i].trim() &&
      !/^(?:#|\||- |```)/.test(lines[i])
    )
      p.push(lines[i++]);
    result.push(`<p>${inline(p.join("\n"))}</p>`);
  }
  return result.join("\n");
}
