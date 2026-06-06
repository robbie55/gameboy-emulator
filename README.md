# GB Emulator

A cycle-aware **Game Boy (DMG) emulator written from scratch in modern C++**, with no
dependencies beyond SDL2 for I/O. The CPU, memory bus, PPU, timers, and interrupt handling
are all implemented by hand from the hardware specifications.

![C++](https://img.shields.io/badge/C%2B%2B-20-blue)
![Build](https://img.shields.io/badge/build-CMake-064F8C)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-in%20development-orange)

<!-- The single most important thing on this page once a game runs:
     drop a GIF of gameplay here. Recruiters scroll to this first.
![Gameplay](docs/gameplay.gif)
-->

## About

This is a from-scratch emulator for the original Game Boy (model DMG), built as a deep-dive
into low-level systems programming: the Sharp LR35902 CPU, a tile-based pixel pipeline,
interrupt-driven timing, and the discipline of matching real hardware behavior against a
specification. Dependencies are kept deliberately minimal — SDL2 handles only the window,
framebuffer, input, and audio output; everything that makes the Game Boy a Game Boy is
implemented directly.

**Target hardware:** Sharp LR35902 (SM83) CPU @ 4.194304 MHz · 64 KB little-endian address
space · 160×144 tile-based PPU at ~59.7 Hz · interrupt-driven timers · OAM sprites · joypad.

## Status & roadmap

v1 is considered complete when **Tetris boots and is playable**. Audio and broader game
support follow afterward.

- [ ] **Skeleton** — CMake build, SDL2 window, framebuffer presentation
- [ ] **CPU** — full instruction set incl. CB-prefixed opcodes (validated against Blargg `cpu_instrs`)
- [ ] **Memory bus (MMU)** — full address-space mapping
- [ ] **Timers & interrupts** — DIV/TIMA/TMA/TAC, IME/IE/IF dispatch (validated against `instr_timing`)
- [ ] **PPU** — background → window → sprites (validated against dmg-acid2)
- [ ] **Joypad** — keyboard-mapped input
- [ ] **v1: Tetris playable** ✅ goal line

Backlog (v2+): APU / audio · MBC1/3/5 for more games · battery-backed save RAM · Mooneye
accuracy suite · save states · debugger UI · Game Boy Color.

## Building

**Prerequisites:** a C++20 compiler (GCC 11+, Clang 13+, or MSVC 2022), CMake 3.20+, and SDL2.

Install SDL2:

```bash
# macOS
brew install sdl2 cmake

# Debian / Ubuntu
sudo apt install libsdl2-dev cmake

# Windows (via vcpkg)
vcpkg install sdl2
```

Build:

```bash
git clone https://github.com/<you>/gb-emulator.git
cd gb-emulator
cmake -B build              # add -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake on Windows
cmake --build build
```

## Usage

```bash
./build/gb path/to/rom.gb
```

**Controls** (default mapping):

| Game Boy | Keyboard |
|---|---|
| D-pad | Arrow keys |
| A | Z |
| B | X |
| Start | Enter |
| Select | Right Shift |

## Architecture

The emulator is split into components that mirror the hardware, coordinated by a top-level
`GameBoy` that steps each subsystem in lockstep with the CPU clock:

| Component | Responsibility |
|---|---|
| `cpu` | LR35902 instruction fetch/decode/execute, registers, flags, interrupt servicing |
| `mmu` | Memory bus — routes reads/writes across ROM, VRAM, WRAM, OAM, I/O, HRAM |
| `ppu` | Scanline-based rendering: background, window, and sprite layers → framebuffer |
| `timer` | DIV/TIMA divider and timer interrupts |
| `cartridge` | ROM loading, header parsing, memory-bank controller logic |
| `joypad` | Input register and button state |
| `gameboy` | Wires the components together and drives the main loop |

## Testing

Correctness is validated against the community's standard hardware test ROMs rather than by
eyeballing games:

- **Blargg's test ROMs** — CPU instructions and instruction timing
- **dmg-acid2** — PPU rendering accuracy
- **Gameboy Doctor** — CPU state-log validation (used before the PPU exists)
- **Mooneye Test Suite** — fine-grained timing (v2)

## References

Built with reference to the excellent open documentation maintained by the Game Boy
development community:

- [Pan Docs](https://gbdev.io/pandocs/) — the canonical hardware reference
- [gbops opcode tables](https://izik1.github.io/gbops/)
- [awesome-gbdev](https://github.com/gbdev/awesome-gbdev)
- Test ROMs by Blargg, Gekkio (Mooneye), and Matt Currie (dmg-acid2)

## License & legal

Released under the MIT License — see [`LICENSE`](LICENSE).

This project contains **no copyrighted Nintendo material**. No game ROMs or boot ROM are
included or distributed; you must supply your own legally obtained ROM files. Emulators
themselves are legal — this one reimplements the hardware from public documentation.
