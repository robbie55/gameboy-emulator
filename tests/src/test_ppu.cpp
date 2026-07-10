#include <doctest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "hardware_constants.hpp"
#include "interrupt_controller.hpp"
#include "ppu.hpp"
#include "ppu_constants.hpp"
#include "ppu_helpers.hpp"
#include "ppu_mode.hpp"

TEST_CASE("Tile Number to Address") {
  // │ unsigned │ 0x00        │ 0x8000           │
  // ├──────────┼─────────────┼──────────────────┤
  // │ unsigned │ 0xFF (255)  │ 0x8FF0           │
  // ├──────────┼─────────────┼──────────────────┤
  // │ signed   │ 0x00        │ 0x9000           │
  // ├──────────┼─────────────┼──────────────────┤
  // │ signed   │ 0x7F (127)  │ 0x97F0           │
  // ├──────────┼─────────────┼──────────────────┤
  // │ signed   │ 0xFF (−1)   │ 0x8FF0           │
  // ├──────────┼─────────────┼──────────────────┤
  // │ signed   │ 0x80 (−128) │ 0x8800

  SUBCASE("Unsigned Translation") {
    uint16_t constexpr kUnsignedExpected00Tile{0x8000};
    uint16_t constexpr kUnsignedExpectedFFTile{0x8FF0};

    const bool is_signed{false};

    uint8_t tile_number{0};
    auto res{ppu::helpers::TileNumberToAddress(tile_number, is_signed)};

    CHECK_EQ(res, kUnsignedExpected00Tile);

    tile_number = 255;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, kUnsignedExpectedFFTile);
  }

  SUBCASE("Signed Translation") {
    uint16_t constexpr kSignedExpected00Tile{0x9000};
    uint16_t constexpr kSignedExpected7FTile{0x97F0};
    uint16_t constexpr kSignedExpectedFFTile{0x8FF0};
    uint16_t constexpr kSignedExpected80Tile{0x8800};

    const bool is_signed{true};

    int8_t tile_number{0};
    auto res{ppu::helpers::TileNumberToAddress(tile_number, is_signed)};

    CHECK_EQ(res, kSignedExpected00Tile);

    tile_number = 127;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, kSignedExpected7FTile);

    tile_number = -1;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, kSignedExpectedFFTile);

    tile_number = -128;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, kSignedExpected80Tile);
  }
}

TEST_CASE("Derive Target Mode") {
  uint8_t ly{ppu::kModeOneScanlineEntry};
  uint16_t dot_counter{};

  CHECK_EQ(ppu::helpers::DeriveTargetMode(ly, dot_counter), PPUMode::kVBlank);

  dot_counter = 255;

  // derivee vblank regardless of dot counter value if ly >= kModeOneScanlineEntry
  CHECK_EQ(ppu::helpers::DeriveTargetMode(ly, dot_counter), PPUMode::kVBlank);

  ly--;
  dot_counter = 0;

  CHECK_EQ(ppu::helpers::DeriveTargetMode(ly, dot_counter), PPUMode::kOAM);

  dot_counter = ppu::kModeTwoDots;

  CHECK_EQ(ppu::helpers::DeriveTargetMode(ly, dot_counter), PPUMode::kDraw);

  dot_counter = (ppu::kModeThreeDots + ppu::kModeTwoDots);

  CHECK_EQ(ppu::helpers::DeriveTargetMode(ly, dot_counter), PPUMode::kHBlank);
}

TEST_CASE("Is Bit Set") {
  uint8_t reg{0x00};

  for (size_t bit{}; bit < 8; ++bit) {
    reg |= (1 << bit);

    CHECK(ppu::helpers::IsBitSet(reg, bit));

    reg &= 0;

    CHECK_FALSE(ppu::helpers::IsBitSet(reg, bit));
  }
}

TEST_CASE("Check and Raise Stat Interrupt") {
  InterruptController controller{};
  uint8_t stat_register{};

  // though we iterate the stat register's bit that's set, we always raise an LCD stat interrupt
  // the ppu set's different bits in the stat reg for different purposes, but the interrupt remains the same
  for (size_t bit{}; bit < 5; ++bit) {
    stat_register |= (1 << bit);

    ppu::helpers::CheckAndRaiseStatInterrupt(stat_register, bit, controller);

    // interrupt enable set to 0, not interrupts
    CHECK_FALSE(controller.queryPendingInterrupt().has_value());

    // enable stat bit for interrupts
    controller.setEnable(static_cast<std::byte>(std::to_integer<uint8_t>(controller.getEnable()) | (1 << interrupts::kLcdStatBit)));

    ppu::helpers::CheckAndRaiseStatInterrupt(stat_register, bit, controller);

    CHECK(controller.queryPendingInterrupt().value() == interrupts::kLcdStatBit);

    // clean up for next iteration
    controller.clearFlagBit(interrupts::kLcdStatBit);
    controller.setEnable(std::byte{0x00});
    stat_register &= 0;

    CHECK_FALSE(controller.queryPendingInterrupt().has_value());
  }
}

TEST_CASE("Memory Accessors") {
  SUBCASE("VRAM Accessors") {
    InterruptController controller{};
    PPU ppu{controller};

    uint16_t addr{1};
    uint8_t val{21};

    ppu.writeVRAM(addr + game_boy_memory::kVRAMStart, val);

    CHECK(ppu.readVRAM(addr + game_boy_memory::kVRAMStart) == val);

    val = 35;

    ppu.writeVRAM(addr + game_boy_memory::kVRAMStart, val);

    CHECK(ppu.readVRAM(addr + game_boy_memory::kVRAMStart) == val);
  }

  SUBCASE("OAM Accessors") {
    InterruptController controller{};
    PPU ppu{controller};

    uint16_t addr{1};
    uint8_t val{21};

    ppu.writeOAM(addr + game_boy_memory::kOAMStart, val);

    CHECK(ppu.readOAM(addr + game_boy_memory::kOAMStart) == val);

    val = 35;

    ppu.writeOAM(addr + game_boy_memory::kOAMStart, val);

    CHECK(ppu.readOAM(addr + game_boy_memory::kOAMStart) == val);
  }
}

TEST_CASE("Register Accessors") {
  SUBCASE("Round Trip Access") {
    InterruptController controller{};
    PPU ppu{controller};

    // we leave out ly and dma registers, guarded by asserts, should never happen and therefore shouldn't be tested
    std::vector<uint16_t> const ppu_io_registers{io_registers::kLCDC,    io_registers::kSCX, io_registers::kSCY,
                                                 io_registers::kLYC,     io_registers::kBGP, io_registers::kOBPOne,
                                                 io_registers::kOBPZero, io_registers::kWX,  io_registers::kWY};

    for (size_t i{}; i < std::size(ppu_io_registers); ++i) {
      auto const addr{ppu_io_registers.at(i)};
      ppu.writeRegister(addr, i);

      CHECK(ppu.readRegister(addr) == i);
    }
  }

  SUBCASE("STAT Register Access") {
    InterruptController controller{};
    PPU ppu{controller};

    // 0000 1101
    uint8_t const raw_val{13};

    // write a raw value of 13, we expect that only bits 3-6 are touched, per the hardware
    // https://gbdev.io/pandocs/STAT.html
    ppu.writeRegister(io_registers::kSTAT, raw_val);

    // the bits that aren't touched via write are hardware controlled. Bit 2 is set to 1 if ly == lyc
    // in the case of a freshly initialized ppu, this case would pass. bit7 is always set to 1, per v1
    // bits 1-0 are controlled by the mode, which is initialized at OAM
    // With the above considerations, along with the fact that only bits 3-6 are written via mask, we expect
    // the following value

    uint8_t const bit7{1 << 7};
    uint8_t const coincidence{1 << 2};
    PPUMode const mode{PPUMode::kOAM};
    uint8_t const val{(raw_val & ppu::stat_bits::kStatWriteableMask) | bit7 | coincidence | static_cast<uint8_t>(mode)};
    CHECK(ppu.readRegister(io_registers::kSTAT) == val);
  }
}

// advance function helpers for tests that rely on it
namespace {
  // ly only observable 0-153, 154 spins an infinite loop
  void AdvanceToLine(PPU& ppu, uint8_t const target_ly) {
    uint8_t const max_t_cycles{255};
    while (ppu.readRegister(io_registers::kLY) != target_ly) {
      ppu.advance(max_t_cycles);
    }
  }

  PPUMode GetCurrentMode(PPU& ppu) {
    uint8_t stat_register{ppu.readRegister(io_registers::kSTAT)};

    // stat derived from mode, covers bits 0-1
    return PPUMode{static_cast<PPUMode>(stat_register & (1 << 1 | 1 << 0))};
  }

  void EnableInterruptBit(InterruptController& controller, uint8_t bit) {
    std::byte current_enable{controller.getEnable()};
    std::byte new_enable{static_cast<uint8_t>(std::to_integer<uint8_t>(current_enable) | 1 << bit)};
    controller.setEnable(new_enable);
  }

  void DisableInterruptBit(InterruptController& controller, uint8_t bit) {
    std::byte current_enable{controller.getEnable()};
    std::byte new_enable{static_cast<uint8_t>(std::to_integer<uint8_t>(current_enable) & ~(1 << bit))};
    controller.setEnable(new_enable);
  }

}  // namespace

TEST_CASE("State Machine") {
  SUBCASE("LY Progression in advance") {
    InterruptController controller{};
    PPU ppu{controller};

    uint8_t target_ly{10};

    AdvanceToLine(ppu, target_ly);

    uint8_t ly_register{ppu.readRegister(io_registers::kLY)};

    CHECK(ly_register == target_ly);

    // off by one, since ly wraps around before hitting scanlines per frame
    target_ly = ppu::kScanlinesPerFrame - 1;

    AdvanceToLine(ppu, target_ly);

    ly_register = ppu.readRegister(io_registers::kLY);

    CHECK(ly_register == target_ly);

    // verify wrap around behavior
    target_ly = 0;

    AdvanceToLine(ppu, target_ly);
    ly_register = ppu.readRegister(io_registers::kLY);

    CHECK(ly_register == target_ly);
  }

  SUBCASE("Mode Progression Handled in advance") {
    InterruptController controller{};
    PPU ppu{controller};

    // expect we start in our starting mode, kOAM
    PPUMode cur_mode{GetCurrentMode(ppu)};

    CHECK(cur_mode == PPUMode::kOAM);

    // advance to mode 3
    ppu.advance(ppu::kModeTwoDots);

    cur_mode = GetCurrentMode(ppu);

    CHECK(cur_mode == PPUMode::kDraw);

    // advance to mode 0
    ppu.advance(ppu::kModeThreeDots);

    cur_mode = GetCurrentMode(ppu);

    CHECK(cur_mode == PPUMode::kHBlank);

    // advance until our ly hits the mode one entry
    AdvanceToLine(ppu, ppu::kModeOneScanlineEntry);

    cur_mode = GetCurrentMode(ppu);

    CHECK(cur_mode == PPUMode::kVBlank);
  }

  SUBCASE("Frame Completion Fires after reaching Mode One") {
    InterruptController controller{};
    PPU ppu{controller};

    // advance until right before entering mode one. Vblank does no rendering, so
    // the render frames ends there
    AdvanceToLine(ppu, ppu::kModeOneScanlineEntry - 1);
    CHECK_FALSE(ppu.isFrameComplete());

    // advance to the next line, enter frame complete
    AdvanceToLine(ppu, ppu::kModeOneScanlineEntry);
    CHECK(ppu.isFrameComplete());
  }

  SUBCASE("Reaching VBlank raises a VBlank Interrupt") {
    InterruptController controller{};
    PPU ppu{controller};

    CHECK_FALSE(controller.queryPendingInterrupt().has_value());

    // enable vblank interrupt bit
    EnableInterruptBit(controller, interrupts::kVblankBit);

    // advance past VBlank entry, raising a VBlank interrupt
    AdvanceToLine(ppu, ppu::kModeOneScanlineEntry + 1);

    CHECK(controller.queryPendingInterrupt().has_value());
    CHECK(controller.queryPendingInterrupt().value() == interrupts::kVblankBit);
  }

  SUBCASE("Mode changes in advance raise STAT Interrupt") {
    InterruptController controller{};
    PPU ppu{controller};

    CHECK_FALSE(controller.queryPendingInterrupt().has_value());

    // enable stat bit
    EnableInterruptBit(controller, interrupts::kLcdStatBit);

    // write to stat, write to mode zero select
    ppu.writeRegister(io_registers::kSTAT, (0x00 | 1 << ppu::stat_bits::kModeZeroSelect));

    // we only check and raise interrupts when our mode changes, so iterate until we change modes to mode zero

    // advance past mode two and three
    ppu.advance(ppu::kModeTwoDots);
    ppu.advance(ppu::kModeThreeDots);

    std::optional<int> optional_interrupt{controller.queryPendingInterrupt()};

    CHECK(optional_interrupt.has_value());
    CHECK(optional_interrupt.value() == interrupts::kLcdStatBit);

    // clear interrupt
    controller.clearFlagBit(interrupts::kLcdStatBit);

    // check that there are no pending interrupts after clearing
    optional_interrupt = controller.queryPendingInterrupt();

    CHECK_FALSE(optional_interrupt.has_value());

    ppu.writeRegister(io_registers::kSTAT, (0x00 | 1 << ppu::stat_bits::kModeTwoSelect));

    // advance to mode 2
    ppu.advance(ppu::kModeZeroDots);
    optional_interrupt = controller.queryPendingInterrupt();

    CHECK(optional_interrupt.has_value());
    CHECK(optional_interrupt.value() == interrupts::kLcdStatBit);

    controller.clearFlagBit(interrupts::kLcdStatBit);

    optional_interrupt = controller.queryPendingInterrupt();

    CHECK_FALSE(optional_interrupt.has_value());

    // enable vblank interrupts
    EnableInterruptBit(controller, interrupts::kVblankBit);
    // disable stat interrupts
    DisableInterruptBit(controller, interrupts::kLcdStatBit);

    // advance to Mode one, which should raise a vblank interrupt on its own
    AdvanceToLine(ppu, ppu::kModeOneScanlineEntry);
    optional_interrupt = controller.queryPendingInterrupt();

    CHECK(optional_interrupt.has_value());
    CHECK(optional_interrupt.value() == interrupts::kVblankBit);

    controller.clearFlagBit(interrupts::kVblankBit);

    optional_interrupt = controller.queryPendingInterrupt();

    CHECK_FALSE(optional_interrupt.has_value());
  }

  SUBCASE("LY Coincidence Triggers a STAT Interrupt") {
    InterruptController controller{};
    PPU ppu{controller};

    // enable stat bit for interrupts
    EnableInterruptBit(controller, interrupts::kLcdStatBit);

    uint8_t ly_c_scanline{2};
    ppu.writeRegister(io_registers::kLYC, ly_c_scanline);

    // advance to our lyc line, ly should equal lyc, and fire off an interrupt
    AdvanceToLine(ppu, ly_c_scanline);

    std::optional<int> optional_interrupt{controller.queryPendingInterrupt()};

    // stat register in ppu isn't written, lyc interrupt shouldn't fire
    CHECK_FALSE(optional_interrupt.has_value());

    // write to stat, write to lyc bit
    ppu.writeRegister(io_registers::kSTAT, (0x00 | 1 << ppu::stat_bits::kLYCIntSelect));

    ly_c_scanline = 22;
    ppu.writeRegister(io_registers::kLYC, ly_c_scanline);

    AdvanceToLine(ppu, ly_c_scanline);
    optional_interrupt = controller.queryPendingInterrupt();

    CHECK(optional_interrupt.has_value());
    CHECK(optional_interrupt.value() == interrupts::kLcdStatBit);
  }

  SUBCASE("LCDC Register Disables PPU When Set") {
    InterruptController controller{};
    PPU ppu{controller};

    // advance to a new ly
    uint8_t ly{1};
    AdvanceToLine(ppu, ly);

    // assert our ly has changed
    CHECK(ppu.readRegister(io_registers::kLY) == ly);

    // clear PPU enable bit in lcdc
    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};
    lcdc &= ~(1 << ppu::lcdc_bits::kLCDandPPUEnable);
    ppu.writeRegister(io_registers::kLCDC, lcdc);

    // advance, guard in advance against PPU Enable should reset mode to HBlank, dot counter, and ly
    // dot counter is fully encapsulated, so check against ly and mode
    ppu.advance(1);
    PPUMode cur_mode{GetCurrentMode(ppu)};

    CHECK(cur_mode == PPUMode::kHBlank);
    CHECK(ppu.readRegister(io_registers::kLY) == 0);
  }
}

namespace {
  void SeedPPUForRendering(PPU& ppu, uint8_t lcdc, uint8_t bgp, uint8_t tile_data, uint16_t map_start) {
    // we want our tile map entry to be the map zero start (0x9800), kBGTileMapZeroStart, so we switch off that bit
    // see ppu.renderScanline()
    ppu.writeRegister(io_registers::kLCDC, lcdc);

    // ppu initializes ly_ to 0, so we leave that alone
    // set BGP to identity mapping, concatenate each unique ID -> 3 2 1 0
    ppu.writeRegister(io_registers::kBGP, bgp);

    // seed tile data, we read from 0x9800 (map zero entry) since our ly_ is 0 (tile_row = ly_ / 8), and for the first iteration, x is zero (tile_col
    // = x / 8) our tile_number is read from vram, using : tile_map_entry_addr = (bg_tile_map_base + (tile_row * 32) + tile_col);
    // set to 0 so we read the 0th tile
    ppu.writeVRAM(map_start, tile_data);
  }
}  // namespace

TEST_CASE("Rendering") {
  SUBCASE("Full Rendering Cycle") {
    InterruptController controller{};
    PPU ppu{controller};

    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};
    lcdc &= ~(1 << ppu::lcdc_bits::kBGTileMapArea);
    uint8_t const identity_bgp{0b1110'0100};
    uint8_t const tile_data{0};

    SeedPPUForRendering(ppu, lcdc, identity_bgp, tile_data, ppu::kBGTileMapZeroStart);

    uint8_t const low_byte{0x3C};
    uint8_t const high_byte{0x7E};

    // seed our low and high bytes
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart, low_byte);
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart + 1, high_byte);

    // advance to mode two, kDraw
    ppu.advance(ppu::kModeTwoDots);

    auto framebuffer{ppu.framebuffer()};

    std::array<uint8_t const, 8> const expected_buffer{0, 2, 3, 3, 3, 3, 2, 0};

    for (size_t i{}; i < 8; ++i) {
      CHECK(framebuffer[i] == expected_buffer.at(i));
    }
  }

  SUBCASE("Different BGP's Result in different Shades") {
    // repeat the seeding from above, save the BGP
    InterruptController controller{};
    PPU ppu{controller};

    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};
    lcdc &= ~(1 << ppu::lcdc_bits::kBGTileMapArea);

    // rather than use the identity bgp, we will reverse it -> 0 1 2 3
    // This way, we should expect abs(2 - ID) should be the shades, i.e. 3 -> 1, 2 -> 0, etc.
    uint8_t const reversed_bgp{0b0100'1110};
    uint8_t const tile_data{0};

    SeedPPUForRendering(ppu, lcdc, reversed_bgp, tile_data, ppu::kBGTileMapZeroStart);

    uint8_t const low_byte{0x3C};
    uint8_t const high_byte{0x7E};

    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart, low_byte);
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart + 1, high_byte);

    ppu.advance(ppu::kModeTwoDots);

    auto framebuffer{ppu.framebuffer()};

    // std::array<uint8_t const, 8> const expected_buffer{0, 2, 3, 3, 3, 3, 2, 0};
    std::array<uint8_t const, 8> const expected_buffer{2, 0, 1, 1, 1, 1, 0, 2};

    for (size_t i{}; i < 8; ++i) {
      CHECK(framebuffer[i] == expected_buffer.at(i));
    }
  }

  SUBCASE("Rendering handles Signed Tile Addressing Modes") {
    InterruptController controller{};
    PPU ppu{controller};

    // same seeding procedure as before, this time disable the kBackgroundWindowTileDataArea bit;
    // disabling that bit switches the addressing mode to use signed values
    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};
    lcdc &= ~((1 << ppu::lcdc_bits::kBGTileMapArea) | (1 << ppu::lcdc_bits::kBackgroundWindowTileDataArea));

    uint8_t const identity_bgp{0b1110'0100};
    uint8_t const tile_data{0};

    SeedPPUForRendering(ppu, lcdc, identity_bgp, tile_data, ppu::kBGTileMapZeroStart);

    uint8_t const low_byte{0x3C};
    uint8_t const high_byte{0x7E};

    ppu.writeVRAM(ppu::kBGWindowTileDataAreaSignedStart, low_byte);
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaSignedStart + 1, high_byte);

    ppu.advance(ppu::kModeTwoDots);

    auto framebuffer{ppu.framebuffer()};

    // expect the same shades as the first test, in a different part of vram
    std::array<uint8_t const, 8> const expected_buffer{0, 2, 3, 3, 3, 3, 2, 0};

    for (size_t i{}; i < 8; ++i) {
      CHECK(framebuffer[i] == expected_buffer.at(i));
    }
  }

  SUBCASE("Rendering with Signed Tile addressing using a Negative Tile Number") {
    InterruptController controller{};
    PPU ppu{controller};

    // same seeding procedure as before, this time disable the kBackgroundWindowTileDataArea bit;
    // disabling that bit switches the addressing mode to use signed values
    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};
    lcdc &= ~((1 << ppu::lcdc_bits::kBGTileMapArea) | (1 << ppu::lcdc_bits::kBackgroundWindowTileDataArea));

    uint8_t const identity_bgp{0b1110'0100};
    // negative value for tile data, rather than 0
    uint8_t const tile_data{0x80};
    // see 'Tile Number To Address' test, that's where this addr comes from
    uint16_t const expected_tile_data_addr{0x8800};

    SeedPPUForRendering(ppu, lcdc, identity_bgp, tile_data, ppu::kBGTileMapZeroStart);

    uint8_t const low_byte{0x3C};
    uint8_t const high_byte{0x7E};

    ppu.writeVRAM(expected_tile_data_addr, low_byte);
    ppu.writeVRAM(expected_tile_data_addr + 1, high_byte);

    ppu.advance(ppu::kModeTwoDots);

    auto framebuffer{ppu.framebuffer()};

    // expect the same shades as the first test, in a different part of vram
    std::array<uint8_t const, 8> const expected_buffer{0, 2, 3, 3, 3, 3, 2, 0};

    for (size_t i{}; i < 8; ++i) {
      CHECK(framebuffer[i] == expected_buffer.at(i));
    }
  }

  SUBCASE("Rendering with Signed Tile addressing using a Negative Tile Number") {
    InterruptController controller{};
    PPU ppu{controller};

    // don't disable kBGTileMapArea bit, forces use of map one start, rather than zero
    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};

    uint8_t const identity_bgp{0b1110'0100};

    uint8_t const tile_data{0};

    // pass in map one start
    SeedPPUForRendering(ppu, lcdc, identity_bgp, tile_data, ppu::kBGTileMapOneStart);

    uint8_t const low_byte{0x3C};
    uint8_t const high_byte{0x7E};

    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart, low_byte);
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart + 1, high_byte);

    ppu.advance(ppu::kModeTwoDots);

    auto framebuffer{ppu.framebuffer()};

    // expect the same shades as the first test, in a different part of vram
    std::array<uint8_t const, 8> const expected_buffer{0, 2, 3, 3, 3, 3, 2, 0};

    for (size_t i{}; i < 8; ++i) {
      CHECK(framebuffer[i] == expected_buffer.at(i));
    }
  }

  SUBCASE("Rendering with Non-Zero Scroll X / Scroll Y") {
    InterruptController controller{};
    PPU ppu{controller};

    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};
    lcdc &= ~(1 << ppu::lcdc_bits::kBGTileMapArea);

    uint8_t const identity_bgp{0b1110'0100};
    uint8_t const tile_data{0};

    SeedPPUForRendering(ppu, lcdc, identity_bgp, tile_data, ppu::kBGTileMapZeroStart);

    // using multiples of 8, this leaves the row/col unshifted, yielding the same result
    uint8_t scroll_x{0x10};
    uint8_t scroll_y{0x80};

    // set scroll x and scroll y
    ppu.writeRegister(io_registers::kSCX, scroll_x);
    ppu.writeRegister(io_registers::kSCY, scroll_y);

    uint8_t const low_byte{0x3C};
    uint8_t const high_byte{0x7E};

    // seed our low and high bytes
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart, low_byte);
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart + 1, high_byte);

    // advance to mode two, kDraw
    ppu.advance(ppu::kModeTwoDots);

    auto framebuffer{ppu.framebuffer()};

    std::array<uint8_t const, 8> const expected_buffer{0, 2, 3, 3, 3, 3, 2, 0};

    for (size_t i{}; i < 8; ++i) {
      CHECK(framebuffer[i] == expected_buffer.at(i));
    }
  }

  SUBCASE("Rendering with Non-Zero Scroll X / Scroll Y, non multiple of 8") {
    InterruptController controller{};
    PPU ppu{controller};

    uint8_t lcdc{ppu.readRegister(io_registers::kLCDC)};
    lcdc &= ~(1 << ppu::lcdc_bits::kBGTileMapArea);

    uint8_t const identity_bgp{0b1110'0100};
    uint8_t const tile_data{0};

    SeedPPUForRendering(ppu, lcdc, identity_bgp, tile_data, ppu::kBGTileMapZeroStart);

    // doesn't divide into 8, modulus leaves row/col shifted from scroll
    uint8_t scroll_x{0x12};
    uint8_t scroll_y{0xA2};

    // set scroll x and scroll y
    ppu.writeRegister(io_registers::kSCX, scroll_x);
    ppu.writeRegister(io_registers::kSCY, scroll_y);

    uint8_t const low_byte{0x3C};
    uint8_t const high_byte{0x7E};

    // Non zero bg_y results in the +4 offset
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart + 4, low_byte);
    ppu.writeVRAM(ppu::kBGWindowTileDataAreaUnsignedStart + 4 + 1, high_byte);

    // advance to mode two, kDraw
    ppu.advance(ppu::kModeTwoDots);

    auto framebuffer{ppu.framebuffer()};

    // scroll_x results in expected shades shifting
    std::array<uint8_t const, 8> const expected_buffer{3, 3, 3, 3, 2, 0, 0, 2};

    for (size_t i{}; i < 8; ++i) {
      CHECK(framebuffer[i] == expected_buffer.at(i));
    }
  }
}
