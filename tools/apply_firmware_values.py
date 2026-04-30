#!/usr/bin/env python3
import pathlib
import re
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
MAIN_CPP = ROOT / "src" / "main.cpp"

ALLOWED_NAMES = {
    "kBaseFreqHz",
    "kPitchAmountHz",
    "kPitchDecaySec",
    "kAmpDecaySec",
    "kLowpassCutoffHz",
    "kResonance",
    "kOutputGain",
    "kRepeatTimeSec",
}

CONSTANT_RE = re.compile(
    r"^\s*constexpr\s+float\s+"
    r"(?P<name>k[A-Za-z0-9_]+)\s*=\s*"
    r"(?P<value>[0-9]+(?:\.[0-9]+)?f?)\s*;\s*$"
)


def read_clipboard():
    try:
        result = subprocess.run(
            ["powershell", "-NoProfile", "-Command", "Get-Clipboard -Raw"],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
        return result.stdout
    except (FileNotFoundError, subprocess.CalledProcessError):
        return sys.stdin.read()


def parse_constants(text):
    constants = {}
    for line in text.splitlines():
        match = CONSTANT_RE.match(line)
        if not match:
            continue

        name = match.group("name")
        if name in ALLOWED_NAMES:
            value = match.group("value")
            constants[name] = value if value.endswith("f") else value + "f"

    missing = sorted(ALLOWED_NAMES - constants.keys())
    if missing:
        raise ValueError("Missing constants: " + ", ".join(missing))

    return constants


def apply_constants(constants):
    source = MAIN_CPP.read_text(encoding="utf-8")

    def replace(match):
        name = match.group("name")
        if name not in constants:
            return match.group(0)

        prefix = match.group("prefix")
        return f"{prefix}{constants[name]};"

    pattern = re.compile(
        r"^(?P<prefix>\s*constexpr\s+float\s+"
        r"(?P<name>k[A-Za-z0-9_]+)\s*=\s*)"
        r"[0-9]+(?:\.[0-9]+)?f?\s*;",
        re.MULTILINE,
    )

    updated = pattern.sub(replace, source)
    if updated == source:
        raise RuntimeError("No constants were changed in src/main.cpp.")

    MAIN_CPP.write_text(updated, encoding="utf-8")


def main():
    constants = parse_constants(read_clipboard())
    apply_constants(constants)

    print("Updated src/main.cpp with copied firmware values:")
    for name in sorted(constants):
        print(f"  {name} = {constants[name]}")


if __name__ == "__main__":
    raise SystemExit(main())
