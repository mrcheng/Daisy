# Agent Notes

This repo is a Daisy Seed C++ firmware project for the Kicki Danielsson bassdrum. Treat the repo root as the project root.

## Project Shape

- Target hardware: Electro-Smith Daisy Seed.
- Main firmware source: `src/main.cpp`.
- Build entry point: `Makefile`.
- Dependencies are git submodules:
  - `libDaisy`
  - `DaisySP`
- Do not create or depend on `~/Desktop/Daisy/DaisyExamples` for this project.
- Do not vendor/copy library contents into the repo outside the submodules.

## Toolchain

Use the official Daisy Windows toolchain workflow.

On Windows, use Git Bash for build and flash commands. PowerShell can inspect files, but Daisy Makefiles and VS Code tasks should be validated through Git Bash.

Expected commands:

```bash
git --version
make --version
arm-none-eabi-gcc --version
dfu-util --version
python --version
```

If tools are missing, fix PATH and ask the user to reopen Git Bash and VS Code before rechecking.

## Build And Flash

Build from the repo root:

```bash
make
```

Clean firmware output:

```bash
make clean
```

Clean firmware and libraries:

```bash
make clean-all
```

Flash over USB DFU:

```bash
make program-dfu
```

Only run `make program-dfu` when the user has connected the Daisy Seed and put it into DFU bootloader mode.

## VS Code

The workspace includes `.vscode/tasks.json` and `.vscode/c_cpp_properties.json`.

Use the existing tasks. Keep Git Bash as the default integrated terminal on Windows.

## Coding Guidelines

- Keep the bassdrum firmware simple and readable.
- Prefer libDaisy and DaisySP APIs over custom low-level hardware code.
- Do not use C++ libraries, OS APIs, dynamic allocation patterns, threading, file I/O, networking, exceptions, RTTI, or runtime features that are not compatible with Daisy Seed embedded firmware.
- Any code added to `src/` must compile for the Daisy Seed target with the repo Makefile and the ARM embedded toolchain.
- Keep audio callback code allocation-free and fast.
- Use conservative output gain by default.
- Do not commit build outputs from `build/`.
- Do not modify submodule internals unless the task explicitly requires library work.

## Verification

Before reporting a firmware change complete, run:

```bash
make
```

Mention whether flashing was performed. If not, explain that hardware DFU mode is required.
