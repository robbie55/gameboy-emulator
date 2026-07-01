#include <doctest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "hardware_constants.hpp"
#include "interrupt_controller.hpp"
#include "ppu.hpp"
#include "ppu_constants.hpp"
#include "ppu_helpers.hpp"

TEST_CASE("Tile Number to Address") {
  SUBCASE("Unsigned Translation") {
    const bool is_signed{false};
    const auto bytes_per_tile{16};
    const uint16_t unsigned_addr_base{ppu::kBGWindowTileDataAreaUnsignedStart};

    uint8_t tile_number{0};
    auto res{ppu::helpers::TileNumberToAddress(tile_number, is_signed)};

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));

    tile_number = 1;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));

    tile_number = 255;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));
  }

  SUBCASE("Signed Translation") {
    const bool is_signed{true};
    const auto bytes_per_tile{16};
    const uint16_t unsigned_addr_base{ppu::kBGWindowTileDataAreaSignedStart};

    int8_t tile_number{0};
    auto res{ppu::helpers::TileNumberToAddress(tile_number, is_signed)};

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));

    tile_number = 1;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));

    tile_number = 127;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));

    tile_number = -1;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));

    tile_number = -128;

    res = ppu::helpers::TileNumberToAddress(tile_number, is_signed);

    CHECK_EQ(res, unsigned_addr_base + (tile_number * bytes_per_tile));
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
  }
}

TEST_CASE("Memorry Accessors") {
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
  void AdvanceToLine(PPU& ppu, uint16_t const target_ly) {
    uint8_t const max_t_cycles{255};
    while (ppu.readRegister(io_registers::kLY) != target_ly) {
      ppu.advance(max_t_cycles);
    }
  }

  PPUMode GetCurrentMode(PPU& ppu) {
    uint16_t stat_register{ppu.readRegister(io_registers::kSTAT)};

    // stat derived from mode, covers bits 0-1
    return PPUMode{static_cast<PPUMode>(stat_register & (1 << 1 | 1 << 0))};
  }

}  // namespace

TEST_CASE("State Machine") {
  SUBCASE("LY Progression") {
    InterruptController controller{};
    PPU ppu{controller};

    uint16_t target_ly{10};

    AdvanceToLine(ppu, target_ly);

    uint16_t ly_register{ppu.readRegister(io_registers::kLY)};

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

  SUBCASE("Mode Progression") {
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

  SUBCASE("Frame Completion") {
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

  SUBCASE("VBlank Interrupt") {
    InterruptController controller{};
    PPU ppu{controller};

    CHECK_FALSE(controller.queryPendingInterrupt().has_value());

    std::byte current_enable{controller.getEnable()};

    std::byte new_enable{static_cast<uint8_t>(std::to_integer<uint8_t>(current_enable) | 1 << interrupts::kVblankBit)};

    // enable VBlank bit
    controller.setEnable(new_enable);

    // advance past VBlank entry, raising a VBlank interrupt
    AdvanceToLine(ppu, ppu::kModeOneScanlineEntry + 1);

    CHECK(controller.queryPendingInterrupt().has_value());
  }
}
