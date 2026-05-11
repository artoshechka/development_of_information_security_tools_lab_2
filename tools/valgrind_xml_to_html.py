#!/usr/bin/env python3

import argparse
import html
import sys
from pathlib import Path
import xml.etree.ElementTree as ET


def _text(elem, default=""):
    if elem is None or elem.text is None:
        return default
    return elem.text.strip()


def _get_child_text(parent, tag, default=""):
    if parent is None:
        return default
    return _text(parent.find(tag), default)


def _parse_frames(stack_elem):
    frames = []
    if stack_elem is None:
        return frames
    for frame in stack_elem.findall("frame"):
        frames.append(
            {
                "ip": _get_child_text(frame, "ip"),
                "obj": _get_child_text(frame, "obj"),
                "fn": _get_child_text(frame, "fn"),
                "dir": _get_child_text(frame, "dir"),
                "file": _get_child_text(frame, "file"),
                "line": _get_child_text(frame, "line"),
            }
        )
    return frames


def _format_frames(frames):
    if not frames:
        return ""
    lines = []
    for frame in frames:
        fn = frame.get("fn") or "?"
        location_parts = []
        if frame.get("file"):
            location_parts.append(frame["file"])
        if frame.get("line"):
            location_parts.append("L" + frame["line"])
        location = ":".join(location_parts)
        obj = frame.get("obj") or ""
        line = f"{fn}"
        if location:
            line += f" ({location})"
        if obj:
            line += f" [{obj}]"
        lines.append(line)
    return "\n".join(lines)


def parse_valgrind_xml(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()

    preamble_lines = [
        _text(line) for line in root.findall("./preamble/line") if _text(line)
    ]

    command_line = ""
    if preamble_lines:
        for line in preamble_lines:
            if line.startswith("Command:"):
                command_line = line
                break

    status_elem = root.findall("status")
    status_state = ""
    status_time = ""
    if status_elem:
        last_status = status_elem[-1]
        status_state = _get_child_text(last_status, "state")
        status_time = _get_child_text(last_status, "time")

    errors = []
    for error in root.findall("error"):
        xwhat = error.find("xwhat")
        xwhat_text = _get_child_text(xwhat, "text")
        leakedbytes = _get_child_text(xwhat, "leakedbytes")
        leakedblocks = _get_child_text(xwhat, "leakedblocks")

        stacks = []
        for stack_elem in error.findall("stack"):
            stacks.append(_parse_frames(stack_elem))

        errors.append(
            {
                "unique": _get_child_text(error, "unique"),
                "tid": _get_child_text(error, "tid"),
                "kind": _get_child_text(error, "kind"),
                "what": _get_child_text(error, "what"),
                "auxwhat": _get_child_text(error, "auxwhat"),
                "xwhat_text": xwhat_text,
                "leakedbytes": leakedbytes,
                "leakedblocks": leakedblocks,
                "stacks": stacks,
            }
        )

    errorcounts = []
    for pair in root.findall("./errorcounts/pair"):
        errorcounts.append(
            {
                "count": _get_child_text(pair, "count"),
                "unique": _get_child_text(pair, "unique"),
            }
        )

    return {
        "preamble": preamble_lines,
        "command": command_line,
        "status_state": status_state,
        "status_time": status_time,
        "errors": errors,
        "errorcounts": errorcounts,
    }


def render_html(title, parsed):
    preamble = "\n".join(html.escape(line) for line in parsed["preamble"]) or ""
    command = html.escape(parsed["command"]) if parsed["command"] else ""
    status_state = html.escape(parsed["status_state"]) if parsed["status_state"] else ""
    status_time = html.escape(parsed["status_time"]) if parsed["status_time"] else ""

    rows = []
    for idx, error in enumerate(parsed["errors"], start=1):
        what = error["what"] or error["xwhat_text"]
        auxwhat = error["auxwhat"] or ""
        leak = ""
        if error["leakedbytes"] or error["leakedblocks"]:
            leak = f"{error['leakedbytes']} bytes, {error['leakedblocks']} blocks"

        stack_text = ""
        if error["stacks"]:
            stack_text = _format_frames(error["stacks"][0])

        rows.append(
            "<tr>"
            f"<td>{idx}</td>"
            f"<td>{html.escape(error['kind'])}</td>"
            f"<td>{html.escape(what)}</td>"
            f"<td>{html.escape(auxwhat)}</td>"
            f"<td>{html.escape(leak)}</td>"
            f"<td><pre>{html.escape(stack_text)}</pre></td>"
            "</tr>"
        )

    errorcount_rows = []
    for pair in parsed["errorcounts"]:
        errorcount_rows.append(
            "<tr>"
            f"<td>{html.escape(pair['unique'])}</td>"
            f"<td>{html.escape(pair['count'])}</td>"
            "</tr>"
        )

    error_table = "\n".join(rows) or "<tr><td colspan=\"6\">No errors</td></tr>"
    errorcounts_table = "\n".join(errorcount_rows) or "<tr><td colspan=\"2\">No error counts</td></tr>"

    return f"""<!doctype html>
<html lang=\"en\">
<head>
  <meta charset=\"utf-8\" />
  <title>{html.escape(title)}</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 24px; color: #1b1b1b; }}
    h1 {{ margin-bottom: 6px; }}
    .meta {{ color: #444; margin-bottom: 16px; }}
    pre {{ white-space: pre-wrap; margin: 0; }}
    table {{ border-collapse: collapse; width: 100%; margin-bottom: 24px; }}
    th, td {{ border: 1px solid #ddd; padding: 8px; vertical-align: top; }}
    th {{ background: #f3f3f3; text-align: left; }}
    .small {{ font-size: 12px; color: #666; }}
  </style>
</head>
<body>
  <h1>{html.escape(title)}</h1>
  <div class=\"meta\">
    <div><strong>Errors:</strong> {len(parsed['errors'])}</div>
    <div><strong>Status:</strong> {status_state or "-"} {status_time}</div>
    <div><strong>Command:</strong> {command or "-"}</div>
  </div>
  <div class=\"small\"><pre>{preamble}</pre></div>

  <h2>Error Summary</h2>
  <table>
    <thead>
      <tr>
        <th>Unique</th>
        <th>Count</th>
      </tr>
    </thead>
    <tbody>
      {errorcounts_table}
    </tbody>
  </table>

  <h2>Errors</h2>
  <table>
    <thead>
      <tr>
        <th>#</th>
        <th>Kind</th>
        <th>What</th>
        <th>Aux</th>
        <th>Leak</th>
        <th>Stack (first)</th>
      </tr>
    </thead>
    <tbody>
      {error_table}
    </tbody>
  </table>
</body>
</html>
"""


def convert_file(xml_path, output_dir):
    parsed = parse_valgrind_xml(xml_path)
    title = xml_path.stem
    html_text = render_html(title, parsed)
    output_path = output_dir / (xml_path.stem + ".html")
    output_path.write_text(html_text, encoding="utf-8")
    return output_path


def _is_min_dont_care(path):
    normalized = path.stem.lower().replace("-", "_")
    return "min_dont_care" in normalized


def main():
    parser = argparse.ArgumentParser(
        description="Convert Valgrind XML reports to simple HTML tables."
    )
    parser.add_argument(
        "--input",
        required=True,
        help="Folder with XML files",
    )
    parser.add_argument(
        "--output",
        help="Output folder for HTML files (default: input folder)",
    )
    args = parser.parse_args()

    input_dir = Path(args.input).expanduser().resolve()
    output_dir = (
        Path(args.output).expanduser().resolve()
        if args.output
        else input_dir
    )

    if not input_dir.is_dir():
        print(f"Input folder not found: {input_dir}", file=sys.stderr)
        return 2

    output_dir.mkdir(parents=True, exist_ok=True)

    xml_files = sorted(input_dir.glob("*.xml"))
    xml_files = [path for path in xml_files if _is_min_dont_care(path)]
    if not xml_files:
        print("No min_dont_care XML files found.")
        return 0

    for xml_path in xml_files:
        try:
            out_path = convert_file(xml_path, output_dir)
            print(f"Wrote {out_path}")
        except ET.ParseError as exc:
            print(f"Failed to parse {xml_path}: {exc}", file=sys.stderr)
        except Exception as exc:
            print(f"Failed to convert {xml_path}: {exc}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
