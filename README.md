# Kicki Danielsson

A small sine-based bassdrum synth for the Electro-Smith Daisy Seed.

The firmware uses libDaisy and DaisySP to make a simple kick drum with a sine oscillator, pitch drop, amplitude decay, lowpass filtering, and resonance.

## Repository Layout

```text
.
|-- src/main.cpp       # Kicki Danielsson firmware
|-- preview/index.html # Browser bassdrum preview
|-- Makefile          # Build/flash entry point
|-- libDaisy          # Electro-Smith library submodule
|-- DaisySP           # Electro-Smith DSP library submodule
|-- .vscode           # VS Code tasks and C++ IntelliSense setup
```

`libDaisy` and `DaisySP` are git submodules. They are not copied into this repo, so clone with submodules.

## Requirements

Use Windows with Git Bash, VS Code, and the official Daisy Windows toolchain.

Install:

- Git for Windows
- Daisy Windows Toolchain Installer
- VS Code
- Python from python.org, not Microsoft Store
- VS Code extensions:
  - C/C++
  - Cortex-Debug
  - Makefile Tools

Official setup docs:

- https://daisy.audio/tutorials/toolchain-windows/
- https://daisy.audio/tutorials/Understanding-the-Toolchain/
- https://daisy.audio/tutorials/cpp-dev-env/

## Clone

```bash
git clone --recurse-submodules https://github.com/mrcheng/Daisy.git
cd Daisy
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

## Verify Toolchain

Open Git Bash and run:

```bash
git --version
make --version
arm-none-eabi-gcc --version
dfu-util --version
python --version
code --version
```

If any command is not found, fix PATH, then close and reopen Git Bash and VS Code.

Typical Windows PATH entries include:

```text
C:\Program Files\Git\cmd
C:\Program Files\DaisyToolchain\bin
C:\Users\<you>\AppData\Local\Programs\Python\Python313\
C:\Users\<you>\AppData\Local\Programs\Python\Python313\Scripts\
C:\Users\<you>\AppData\Local\Programs\Microsoft VS Code\bin
```

Python must resolve to the python.org install, not the Microsoft Store alias.

## Build

From Git Bash:

```bash
make
```

This builds `libDaisy`, `DaisySP`, and then the firmware.

Build outputs are written to `build/`:

```text
build/KickiDanielsson.elf
build/KickiDanielsson.hex
build/KickiDanielsson.bin
```

To clean only the firmware build:

```bash
make clean
```

To clean firmware and libraries:

```bash
make clean-all
```

To rebuild firmware and refresh the browser memory report:

```bash
python tools/update_memory_report.py
```

## Flash With USB DFU

Connect the Daisy Seed over USB.

Put the Daisy Seed into bootloader mode:

1. Hold `BOOT`.
2. Press and release `RESET`.
3. Release `BOOT`.

Then run:

```bash
make program-dfu
```

Normal edit/build/flash loop:

```bash
make
make program-dfu
```

## VS Code

Open the repo folder:

```bash
code .
```

This repo includes VS Code tasks:

- `build`
- `clean`
- `clean-all`
- `program-dfu`
- `build_and_program_dfu`

Use `Terminal > Run Build Task...` or press `Ctrl+Shift+B` to run the default build task.

The workspace is configured to use Git Bash as the default integrated terminal on Windows.

## Browser Preview

The Daisy firmware cannot run directly on the PC. For quick sound-design work before flashing, this repo includes a browser preview that mirrors the kick parameters with the Web Audio API.

Open:

```text
preview/index.html
```

Or from the repo root on Windows:

```powershell
start preview/index.html
```

The preview lets you:

- Trigger the bassdrum.
- Repeat the bassdrum every 750 ms.
- Set BPM manually.
- Tap a tempo for the repeat rate.
- Change base pitch.
- Change pitch drop and pitch decay.
- Change body decay.
- Change lowpass cutoff and resonance.
- Change output gain.
- Watch the real browser output VU meter.
- Watch the latest Daisy Seed memory usage from `preview/memory-report.js`.

Use this for fast listening and parameter experiments. When a sound is worth keeping, copy the matching values into `src/main.cpp`, build with `make`, and flash with `make program-dfu`.

After firmware changes, refresh the memory report with:

```bash
python tools/update_memory_report.py
```

## Editing The Synth

The firmware is in `src/main.cpp`.

Change the final body pitch here:

```cpp
constexpr float kBaseFreqHz = 48.0f;
```

Change the pitch drop here:

```cpp
constexpr float kPitchAmountHz = 112.0f;
```

Change the pitch decay and body decay here:

```cpp
constexpr float kPitchDecaySec = 0.055f;
constexpr float kAmpDecaySec   = 0.36f;
```

Change the lowpass filter here:

```cpp
constexpr float kLowpassCutoffHz = 760.0f;
constexpr float kResonance       = 0.62f;
```

Change the output volume here:

```cpp
constexpr float kOutputGain = 0.7f;
```

Keep volume conservative when using headphones or speakers.

## Setup Prompt For A Friend

If someone wants an AI assistant to set up their Windows machine for this repo, they can paste this:

```text
You are setting up my Windows computer for C++ synth/audio programming on the Electro-Smith Daisy Seed using VS Code.

Use the official Electro-Smith Daisy Windows toolchain workflow. Use Git Bash as the terminal. Target board is Daisy Seed.

Install and verify:
- Git for Windows and Git Bash
- Daisy Windows Toolchain Installer
- VS Code
- Python from python.org, not Microsoft Store
- VS Code extensions: C/C++, Cortex-Debug, Makefile Tools

After installing, open Git Bash and verify:
git --version
make --version
arm-none-eabi-gcc --version
dfu-util --version
python --version
code --version

If any command is not found, fix PATH, then reopen Git Bash and VS Code and explain exactly what changed.

Clone and build this repo:
git clone --recurse-submodules https://github.com/mrcheng/Daisy.git
cd Daisy
make

Configure VS Code:
- Make Git Bash the default integrated terminal.
- Confirm VS Code can run the build task.

To flash the Daisy Seed, put it into DFU bootloader mode and run:
make program-dfu
```
