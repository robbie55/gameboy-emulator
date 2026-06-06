# DESIGN.md

Architecture decisions and rationale for this emulator. This is a **living document** —
record decisions here as you make them. Decisions already settled are logged at the bottom;
the open ones below are framed with their tradeoffs for *you* to resolve (consistent with
the learning approach in `CLAUDE.md` — the reasoning is yours to do, not to receive).

## Design principles

1. **Module boundaries mirror the hardware.** `cpu`, `mmu`, `ppu`, `timer`, `cartridge`,
   `joypad`, coordinated by `gameboy`. If you can't point at the chip a class represents,
   reconsider the class.
2. **Minimal dependencies.** SDL2 for I/O only. The emulation core is hand-written.
3. **Correctness is validated, not asserted.** Test ROMs are the source of truth (see TECH.md
   and README). "It looks right" is not a passing state.
4. **Abstract only where it earns its cost.** This is the central lesson of *C++ Software
   Design* applied directly: the cartridge/MBC layer deserves a clean polymorphic seam; the
   CPU dispatch and PPU inner loops are hot paths where a flat, boring implementation wins.
   Be able to justify every abstraction in interview terms.

## System synchronization (the first big decision)

Every subsystem advances in step with the CPU clock. Two models:

- **Instruction-stepped catch-up** — execute one CPU instruction, measure its T-cycles, then
  advance the PPU and timers by that many cycles. Simpler; gets games running fast.
- **Cycle-stepped (tick) accuracy** — every component advances one (or four) cycles at a time
  in lockstep. More accurate for mid-instruction timing effects; harder to write.

*Tradeoff:* catch-up is enough to make Tetris run and is the right v1 choice; some accuracy
test ROMs and a few games with tight timing eventually want finer granularity. **Recommended
v1 path: instruction-stepped catch-up**, with subsystems structured so you could tighten the
granularity later without a rewrite. Decide, then log it.

## Open component decisions

**CPU opcode dispatch** — giant `switch` vs. a function/handler table. The `switch` is simple,
fast, and the compiler optimizes it well; a table is more data-driven but adds indirection.
Most first emulators use the switch. Your call — weigh readability against the 500-entry size.

**Register pairing (AF/BC/…)** — union type-punning vs. explicit `bc()`/`set_bc()` accessors.
The union is popular but leans on murky aliasing rules; accessors are portable and trivial.
(See the gotcha note in `CLAUDE.md`.) Decide *why* before you decide which.

**MMU read/write dispatch** — routing an address to its region. Resist the urge to make this a
hierarchy of handler objects; a flat dispatch on the high address bits is faster and clearer.
This is a textbook "don't abstract the hot path" case.

**Cartridge / MBC** — *this* is where polymorphism belongs. Define a cartridge interface
(`read`/`write`) and implement it per controller. v1 needs only ROM-only, but design the seam
now so MBC1/3/5 slot in later without touching the MMU. This is your Strategy-pattern moment.

**PPU rendering** — scanline-based vs. pixel-FIFO. Scanline rendering is simpler and sufficient
for v1 and most games; the pixel FIFO is hardware-accurate and needed for certain effects and
stricter test ROMs. **Recommended v1: scanline**, FIFO as a possible v2 refactor.

## Decision log (ADR)

Append a row whenever you settle something. Keep the rationale honest — future-you will want it.

| Date | Decision | Rationale | Alternatives considered |
|---|---|---|---|
| 2026-06-06 | Language: C++20 | Modern stdlib (`std::span`, `<bit>`), broad compiler support | C++17 (acceptable floor) |
| 2026-06-06 | Deps: SDL2 + CMake only | Core is the learning goal; keep surface minimal | SFML, raylib, GLFW+GL |
| 2026-06-06 | v1 scope = Tetris playable, sound off | Shortest path to a complete, demoable result; ROM-only cart | Targeting audio/more games in v1 |
| 2026-06-06 | Validate CPU before building PPU | Isolates the hardest subsystem; avoids debugging ten things at once | Build straight through to graphics |
| | | | |
