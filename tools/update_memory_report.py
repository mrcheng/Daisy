#!/usr/bin/env python3
import datetime as dt
import json
import os
import pathlib
import re
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "preview" / "memory-report.js"


MEMORY_RE = re.compile(
    r"^\s*(?P<name>[A-Z0-9_]+):\s+"
    r"(?P<used>[0-9.]+)\s+(?P<used_unit>[A-Z]+)\s+"
    r"(?P<total>[0-9.]+)\s+(?P<total_unit>[A-Z]+)\s+"
    r"(?P<percent>[0-9.]+)%"
)


UNIT_SCALE = {
    "B": 1,
    "KB": 1024,
    "MB": 1024 * 1024,
    "GB": 1024 * 1024 * 1024,
}


def to_bytes(value, unit):
    return int(float(value) * UNIT_SCALE[unit])


def format_bytes(value):
    if value == 0:
        return "0 B"

    for unit, scale in (("MB", 1024 * 1024), ("KB", 1024)):
        if value >= scale and value % scale == 0:
            return f"{value // scale} {unit}"
        if value >= scale:
            return f"{value / scale:.1f} {unit}"

    return f"{value} B"


def run(command):
    return subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def build_firmware():
    bash = pathlib.Path("C:/Program Files/Git/bin/bash.exe")
    if os.name == "nt" and bash.exists():
        return run([str(bash), "-lc", "make clean && make"])

    return run(["sh", "-lc", "make clean && make"])


def main():
    build = build_firmware()
    sys.stdout.write(build.stdout)
    if build.returncode != 0:
        return build.returncode

    regions = []
    for line in build.stdout.splitlines():
        match = MEMORY_RE.match(line)
        if not match:
            continue

        item = match.groupdict()
        used_bytes = to_bytes(item["used"], item["used_unit"])
        total_bytes = to_bytes(item["total"], item["total_unit"])
        regions.append(
            {
                "name": item["name"],
                "usedBytes": used_bytes,
                "totalBytes": total_bytes,
                "used": format_bytes(used_bytes),
                "total": format_bytes(total_bytes),
                "percent": float(item["percent"]),
            }
        )

    if not regions:
        raise RuntimeError("Could not find linker memory usage in build output.")

    report = {
        "target": "KickiDanielsson",
        "generatedAt": dt.datetime.now().astimezone().isoformat(timespec="seconds"),
        "regions": regions,
    }

    OUT.write_text(
        "window.KICKI_MEMORY_REPORT = "
        + json.dumps(report, indent=4)
        + ";\n",
        encoding="utf-8",
    )
    print(f"Wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
