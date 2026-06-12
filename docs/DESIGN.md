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

> **→ RESOLVED (2026-06-10): instruction-stepped catch-up.** See the decision log.

## Component decisions  *(all resolved 2026-06-10 — see the decision log below; reasoning retained here)*

**CPU opcode dispatch** — giant `switch` vs. a function/handler table. The `switch` is simple,
fast, and the compiler optimizes it well; a table is more data-driven but adds indirection.
Most first emulators use the switch. **→ RESOLVED: `switch` (v1, for now); no Strategy/injection.**

**Register pairing (AF/BC/…)** — union type-punning vs. explicit `bc()`/`set_bc()` accessors.
The union is popular but leans on murky aliasing rules; accessors are portable and trivial.
(See the gotcha note in `CLAUDE.md`.) Decide *why* before you decide which. **→ RESOLVED: explicit accessors (v1, for now) — union type-punning is UB + a host-endianness trap.**

**MMU read/write dispatch** — routing an address to its region. Resist the urge to make this a
hierarchy of handler objects; a flat dispatch on the high address bits is faster and clearer.
This is a textbook "don't abstract the hot path" case. **→ RESOLVED: flat concrete dispatch; MMU holds devices by concrete type (option A), edges one-way `mmu → device`.**

**Cartridge / MBC** — *this* is where polymorphism belongs. Define a cartridge interface
(`read`/`write`) and implement it per controller. v1 needs only ROM-only, but design the seam
now so MBC1/3/5 slot in later without touching the MMU. This is your Strategy-pattern moment. **→ RESOLVED: cartridge interface at TOP — the one sanctioned polymorphic seam.**

**PPU rendering** — scanline-based vs. pixel-FIFO. Scanline rendering is simpler and sufficient
for v1 and most games; the pixel FIFO is hardware-accurate and needed for certain effects and
stricter test ROMs. **Recommended v1: scanline**, FIFO as a possible v2 refactor. **→ RESOLVED: scanline (v1, for now); FIFO is top of the v2 list.**

## Decision log (ADR)

Append a row whenever you settle something. Keep the rationale honest — future-you will want it.

| Date | Decision | Rationale | Alternatives considered |
|---|---|---|---|
| 2026-06-06 | Language: C++20 | Modern stdlib (`std::span`, `<bit>`), broad compiler support | C++17 (acceptable floor) |
| 2026-06-06 | Deps: SDL2 + CMake only | Core is the learning goal; keep surface minimal | SFML, raylib, GLFW+GL |
| 2026-06-06 | v1 scope = Tetris playable, sound off | Shortest path to a complete, demoable result; ROM-only cart | Targeting audio/more games in v1 |
| 2026-06-06 | Validate CPU before building PPU | Isolates the hardest subsystem; avoids debugging ten things at once | Build straight through to graphics |
| 2026-06-10 | MMU stays concrete (no polymorphic seam); CPU depends on its read/write contract directly | A stable *contract* needs no abstraction (cf. `std::vector` — concrete yet apex-stable). An abstraction only buys substitutability, and there is exactly one DMG bus forever (one implementation). It is also the hottest path (multiple accesses/instruction) — the textbook "don't abstract the hot path." | Abstract `MemoryBus` interface at TOP with CPU depending on the abstraction (rejected: a seam with one implementor is ceremony + virtual-call overhead on the hot path) |
| 2026-06-10 | Interrupt state (`IF`/`IE`) lives in a dedicated controller node at TOP | Stateful, but stable *interface*; depends on nothing in MIDDLE, so interrupt sources (timer/ppu/joypad) and the CPU all depend *up* on it — breaking would-be sideways coupling between sources | Fold `IF`/`IE` into the MMU (rejected: creates sideways MMU↔source coupling the mediator split is meant to avoid) |
| 2026-06-10 | MMU routes to devices concretely (option A): holds distinct typed members (`ppu`, `timer`, `joypad`), calls non-virtual accessors; edges are one-way `mmu → device`, each device owns its mapped state | Change profile is shallow: ≈one known memory-mapped addition (the APU), accommodated by a one-line branch; the concrete→abstract refactor is cheap and reversible. A device-interface seam would pay recurring hot-path + complexity cost to save a trivial one-time edit — a bad trade, and over-abstraction is itself bad design. **Revisit trigger:** if a 2nd/3rd memory-mapped device joins the APU, re-run the abstract-vs-concrete pivot. | Abstract `IMemoryMappedDevice` seam with range registration (rejected for v1/v2: ceremony + virtual dispatch on the hottest path for a single shallow axis of change) |
| 2026-06-10 | Architecture's vertical axis = dependency **stability**, not control flow | Most consistent with general system design (DIP / Stable Dependencies / Stable Abstractions); makes the diagram near-permanent since hardware boundaries are frozen. Consequence: source deps point UP to the stable apex, control flow points DOWN, so the orchestrator sits at the BOTTOM | Organize by control flow / "who's in charge" (rejected: puts `gameboy` at top, breaks the deps-point-up invariant) |
| 2026-06-10 | Middle modules never communicate directly; all cross-module communication funneled through **two mediators** | Concentrates coupling in designated hubs; subsystems stay mutually ignorant. The split is hardware-dictated: `mmu` = data-bus mediator (synchronous, address-based), `gameboy` = temporal mediator (advances subsystems by cycle count). `cpu → mmu` is the one sanctioned spine edge | `gameboy` mediates everything (rejected: cannot sit on the bus for every memory access); free peer-to-peer calls (rejected: couples subsystems) |
| 2026-06-10 | Synchronization model = instruction-stepped catch-up | `cpu.step()` executes one instruction and returns its T-cycles; `gameboy` then advances timer/ppu by that count. Simpler than cycle-stepped, sufficient for Tetris/v1; structure subsystems so granularity can tighten later without a rewrite | Cycle-stepped (tick) lockstep — deferred to v2 if accuracy ROMs demand it |
| 2026-06-10 | Interrupt servicing lives in the CPU (polled at top of each `step`, gated by IME & IE & IF); `gameboy` uninvolved | Sources raise a bit in the TOP interrupt controller during their advance; the CPU collects on its next step, so source and CPU stay mutually ignorant. A bit set in iteration *i* is serviced in *i+1* (matches real dispatch latency) | `gameboy` detects pending interrupts and dispatches (rejected: needless orchestration; re-couples) |
| 2026-06-10 | CPU reads/writes `IF`/`IE` through the MMU (memory-mapped at `FF0F`/`FFFF`), not via a direct dependency on the interrupt controller | Keeps the MMU as the CPU's single outward dependency; `FF0F`/`FFFF` route like any other address. The interrupt controller remains a TOP node that timer/ppu/joypad depend on to *raise* | CPU depends on the interrupt controller directly (rejected: a second outward CPU dependency for no benefit) |
| 2026-06-10 | CPU dispatch = `switch` **(v1, for now)** | Fastest path to Blargg validation; the compiler lowers a dense `switch` to a jump table *and* keeps handlers inlined (beats a function-pointer table on the hot path). No Strategy/injection — one frozen ISA, no axis of change; tracing/breakpoints belong in a hook around `step()`, not in dispatch. **Interior — zero diagram impact. Top of the v2 list:** add a metadata table (cycles/length/mnemonic) if/when a debugger is built; consider structured bit-decode as a learning refactor | Function-pointer table now; structured bit-decode now; Strategy-injected handlers (rejected) |
| 2026-06-10 | Register pairing = explicit `bc()`/`set_bc()` accessors **(v1, for now)** | Standard and portable; union type-punning is UB in C++ (may only read the active member) and silently bakes in *host* endianness. Accessors do explicit shift/mask, so they're endianness-independent and the compiler emits identical code. Writing them by hand makes the little-endian layout visible (learning goal). **Interior — zero diagram impact.** Revisit only if profiling ever shows a difference (it won't) | `union` type-punning (rejected: UB + host-endianness trap, for terseness that buys nothing) |
| 2026-06-10 | PPU rendering = scanline (per-line) **(v1, for now)** | Passes the v1 validation target (dmg-acid2 tests rendering correctness, not mid-scanline timing) and runs Tetris; far simpler. Safe to defer FIFO because the PPU interface (advance-by-cycles, raise VBlank/STAT, emit framebuffer) is identical either way — pure interior swap, zero diagram impact — and the working scanline renderer becomes the reference to validate the FIFO against. **Top of the v2 list:** rebuild as pixel-FIFO (richest cycle-accurate-modeling learning in the project) | Pixel-FIFO now (rejected for v1: highest-complexity subsystem, common burnout point, buys mid-scanline effects no v1 target game needs) |
| 2026-06-12 | First build target = a thin **vertical slice** down the dependency spine (constants → cartridge → mmu → cpu → serial) to the Blargg `cpu_instrs` gate; constants (frozen foundation) land first as the apex leaf | Proves the apex contracts end-to-end on the narrowest path before adding breadth; the whole slice depends *up* on the constants leaf, so it ships first. No behavioral test for the constants module — validation is transitive (`cpu_instrs` "Passed" means its addresses/vectors were right). **Constants are slice-scoped:** each subsystem's bit-layout tables (LCDC/STAT/TAC/PPU timing) land in that subsystem's own branch, so nothing arrives here untested/unused | Build each module fully before integrating (rejected: defers integration risk; means debugging many subsystems at once — the exact trap "validate CPU before PPU" avoids) |
| | | | |
