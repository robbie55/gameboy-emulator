# TECH.md

A quick-reference sheet for the **stable constants** you'll look up constantly — memory map,
register bit layouts, timing numbers, interrupt vectors, header fields.

> **Scope note.** This deliberately captures only reference *constants*, not *mechanics*.
> Opcode semantics, exact flag computations, DMA quirks, and timing edge cases are **not**
> here on purpose — derive those from [Pan Docs](https://gbdev.io/pandocs/) and the
> [opcode tables](https://izik1.github.io/gbops/) yourself. That's the part worth learning.

## CPU

- **Chip:** Sharp LR35902 (SM83 core). 8-bit, 16-bit address bus.
- **Clock:** 4.194304 MHz. 1 machine cycle (M-cycle) = 4 T-cycles.
- **Registers:** `A F B C D E H L` (8-bit), paired as `AF BC DE HL` (16-bit); `SP`, `PC` (16-bit).

**Flags register `F`:**

| Bit | 7 | 6 | 5 | 4 | 3–0 |
|---|---|---|---|---|---|
| Flag | Z (zero) | N (subtract) | H (half-carry) | C (carry) | always 0 |

**Post-boot register state** (if you skip the boot ROM and start at `PC=0x0100`):

| Reg | AF | BC | DE | HL | SP | PC |
|---|---|---|---|---|---|---|
| Value | `0x01B0` | `0x0013` | `0x00D8` | `0x014D` | `0xFFFE` | `0x0100` |

> The `H` and `C` bits of `F` actually depend on the cartridge header checksum — see Pan Docs
> "Power-Up Sequence." `0xB0` is the common case.

## Memory map (64 KB, little-endian)

| Range | Region |
|---|---|
| `0000–3FFF` | ROM bank 0 (fixed) |
| `4000–7FFF` | ROM bank N (switchable via MBC) |
| `8000–9FFF` | VRAM (8 KB) |
| `A000–BFFF` | Cartridge RAM (switchable) |
| `C000–DFFF` | WRAM (8 KB) |
| `E000–FDFF` | Echo RAM (mirror — leave unused) |
| `FE00–FE9F` | OAM (40 sprites) |
| `FEA0–FEFF` | Unusable |
| `FF00–FF7F` | I/O registers |
| `FF80–FFFE` | HRAM (127 bytes) |
| `FFFF` | IE (interrupt enable) |

## Key I/O registers

| Addr | Name | Purpose |
|---|---|---|
| `FF00` | P1/JOYP | Joypad |
| `FF01/02` | SB/SC | Serial (used to capture Blargg test output) |
| `FF04` | DIV | Divider |
| `FF05` | TIMA | Timer counter |
| `FF06` | TMA | Timer modulo |
| `FF07` | TAC | Timer control |
| `FF0F` | IF | Interrupt flag |
| `FF40` | LCDC | LCD control |
| `FF41` | STAT | LCD status |
| `FF42/43` | SCY/SCX | BG scroll |
| `FF44` | LY | Current scanline |
| `FF45` | LYC | LY compare |
| `FF46` | DMA | OAM DMA transfer |
| `FF47` | BGP | BG palette |
| `FF48/49` | OBP0/OBP1 | Sprite palettes |
| `FF4A/4B` | WY/WX | Window position |
| `FFFF` | IE | Interrupt enable |

## Interrupts

| Bit (IE/IF) | Source | Vector |
|---|---|---|
| 0 | VBlank | `0x0040` |
| 1 | LCD STAT | `0x0048` |
| 2 | Timer | `0x0050` |
| 3 | Serial | `0x0058` |
| 4 | Joypad | `0x0060` |

Dispatch is gated by the `IME` master flag plus the per-source `IE`/`IF` bits.

## LCDC (`FF40`) bits

| Bit | Meaning |
|---|---|
| 7 | LCD & PPU enable |
| 6 | Window tile-map area (`9800`/`9C00`) |
| 5 | Window enable |
| 4 | BG/Window tile-data area (`8800`/`8000`) |
| 3 | BG tile-map area (`9800`/`9C00`) |
| 2 | OBJ size (8×8 / 8×16) |
| 1 | OBJ enable |
| 0 | BG & Window enable/priority |

## STAT (`FF41`) bits

| Bit | Meaning |
|---|---|
| 6 | LYC int select |
| 5 | Mode 2 (OAM) int select |
| 4 | Mode 1 (VBlank) int select |
| 3 | Mode 0 (HBlank) int select |
| 2 | LYC == LY flag |
| 1–0 | PPU mode (0 HBlank, 1 VBlank, 2 OAM, 3 Draw) |

## PPU timing

- Screen **160×144**, tiles **8×8**, **4 shades** (2 bits/pixel).
- **Scanline = 456 dots.** Lines 0–143 visible, 144–153 VBlank. **Frame = 70224 dots ≈ 59.7 Hz.**
- Mode 2 (OAM scan): 80 dots · Mode 3 (draw): 172–289 (variable) · Mode 0 (HBlank): remainder.
- OAM holds **40 sprites**, max **10 per scanline**.

## TAC (`FF07`) — timer clock select

| Bits 1–0 | Frequency | Period (T-cycles) |
|---|---|---|
| 00 | 4096 Hz | 1024 |
| 01 | 262144 Hz | 16 |
| 10 | 65536 Hz | 64 |
| 11 | 16384 Hz | 256 |

Bit 2 enables the timer.

## Cartridge header (`0100–014F`)

| Range | Field |
|---|---|
| `0100–0103` | Entry point |
| `0104–0133` | Nintendo logo |
| `0134–0143` | Title |
| `0147` | Cartridge type (MBC) |
| `0148` | ROM size |
| `0149` | RAM size |
| `014D` | Header checksum |

**Cartridge type (`0147`):** `0x00` = ROM only (Tetris). `0x01–0x03` = MBC1 (v2). Others per Pan Docs.
