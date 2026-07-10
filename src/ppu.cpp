#include "ppu.hpp"

#include <cassert>
#include <cstddef>

#include "hardware_constants.hpp"
#include "ppu_constants.hpp"
#include "ppu_helpers.hpp"

// we protect the bad addr invariant via asserts, so NOLINT serves for these accessors
uint8_t PPU::readOAM(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd && "PPU::readOAM -> Given an out of bounds addr");

  return oam_[addr - game_boy_memory::kOAMStart];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

void PPU::writeOAM(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd && "PPU::writeOAM -> Given an out of bounds addr");

  oam_[addr - game_boy_memory::kOAMStart] = val;  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

uint8_t PPU::readVRAM(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kVRAMStart && addr <= game_boy_memory::kVRAMEnd && "PPU::readVRAM -> Given an out of bounds addr");

  return vram_[addr - game_boy_memory::  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
               kVRAMStart];
}

void PPU::writeVRAM(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kVRAMStart && addr <= game_boy_memory::kVRAMEnd && "PPU::writeVRAM -> Given an out of bounds addr");

  vram_[addr -  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
        game_boy_memory::kVRAMStart] = val;
}

uint8_t PPU::readRegister(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kPPUIORegistersStart && addr <= game_boy_memory::kPPUIORegistersEnd &&
         "PPU::readRegister-> Given an out of bounds addr");

  switch (addr) {
    case io_registers::kLCDC:
      return lcdc_;
    case io_registers::kSTAT: {
      uint8_t bit7{1 << 7};
      uint8_t coincidence{static_cast<uint8_t>(static_cast<uint8_t>(ly_ == ly_compare_) << 2)};
      return static_cast<uint8_t>(bit7 | stat_int_select_ | coincidence | static_cast<uint8_t>(mode_));
    }
    case io_registers::kSCX:
      return scroll_x_;
    case io_registers::kSCY:
      return scroll_y_;
    case io_registers::kLY:
      return ly_;
    case io_registers::kLYC:
      return ly_compare_;
    case io_registers::kDMA:
      assert(false && "PPU::readRegister -> asked for DMA register, needs to be routed through readOAM");
      break;
    case io_registers::kBGP:
      return background_palette_;
    case io_registers::kOBPZero:
      return obj_sprite_palette_zero_;
    case io_registers::kOBPOne:
      return obj_sprite_palette_one_;
    case io_registers::kWX:
      return window_x_;
    case io_registers::kWY:
      return window_y_;
    default:
      assert(false && "PPU::readRegister -> reached default case, unknown address");
      break;
  }
  return 0x00;
}

void PPU::writeRegister(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kPPUIORegistersStart && addr <= game_boy_memory::kPPUIORegistersEnd &&
         "PPU::readRegister-> Given an out of bounds addr");

  switch (addr) {
    case io_registers::kLCDC:
      lcdc_ = val;
      return;
    case io_registers::kSTAT:
      // stat int select is a mask, only worry about bits 3-6, rest aren't used here
      stat_int_select_ = ppu::stat_bits::kStatWriteableMask & val;
      return;
    case io_registers::kSCX:
      scroll_x_ = val;
      return;
    case io_registers::kSCY:
      scroll_y_ = val;
      return;
    case io_registers::kLY:
      return;
    case io_registers::kLYC:
      ly_compare_ = val;
      return;
    case io_registers::kDMA:
      assert(false && "PPU::writeRegister -> called a write on DMA register, should be written with writeOAM");
      return;
    case io_registers::kBGP:
      background_palette_ = val;
      return;
    case io_registers::kOBPZero:
      obj_sprite_palette_zero_ = val;
      return;
    case io_registers::kOBPOne:
      obj_sprite_palette_one_ = val;
      return;
    case io_registers::kWX:
      window_x_ = val;
      return;
    case io_registers::kWY:
      window_y_ = val;
      return;
    default:
      assert(false && "PPU::readRegister -> Given an address not corresponding to a ppu owned register");
  }
}

void PPU::advanceScanline() {
  dot_counter_ = 0;
  ly_++;

  if (ly_ == ppu::kScanlinesPerFrame) {
    ly_ = 0;
  }

  if (ly_ == ly_compare_) {
    ppu::helpers::CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kLYCIntSelect, interrupt_handler_);
  }
}

uint8_t PPU::getShadeFromTileAddr(uint16_t tile_addr, uint8_t bg_y, uint8_t bg_x) const {
  uint16_t row_addr{static_cast<uint16_t>(tile_addr + ((bg_y % 8) * 2))};

  uint8_t low_byte{readVRAM(row_addr)};
  uint8_t high_byte{readVRAM(row_addr + 1)};

  uint8_t bit_to_extract{static_cast<uint8_t>(7 - (bg_x % 8))};

  uint8_t low_bit{static_cast<uint8_t>((low_byte >> bit_to_extract) & 1)};
  uint8_t high_bit{static_cast<uint8_t>((high_byte >> bit_to_extract) & 1)};

  uint8_t color_id{static_cast<uint8_t>((high_bit << 1) | low_bit)};

  uint8_t shade{static_cast<uint8_t>((background_palette_ >> (color_id * 2)) & 3)};

  return shade;
}

void PPU::renderScanline() {
  // So when the PPU enters mode 3 for line LY, renderScanline does the whole production:
  // 1. read LCDC to know which BG tile map (9800/9C00, bit 3) and which tile-data addressing
  // (8000/8800, bit 4),
  // 2. apply SCX/SCY to figure out which part of the 256×256 background covers this line,
  // 3. read the tile map from VRAM → tile indices,
  // 4. read the tile data from VRAM → the 2-bit color ID for each of the 160 pixels on this line,
  // 5. resolve each ID through BGP → a shade,
  // 6. write those 160 shades into frame_buffer_ at row LY.

  uint16_t bg_tile_map_base{ppu::kBGTileMapZeroStart};
  if (ppu::helpers::IsBitSet(lcdc_, ppu::lcdc_bits::kBGTileMapArea)) {
    bg_tile_map_base = ppu::kBGTileMapOneStart;
  }

  bool is_tile_addressing_signed{!ppu::helpers::IsBitSet(lcdc_, ppu::lcdc_bits::kBackgroundWindowTileDataArea)};

  // background y location
  uint8_t bg_y{static_cast<uint8_t>((scroll_y_ + ly_) % 256)};

  // current row
  auto tile_row{bg_y / 8};

  // iterate left to right on current row
  for (size_t x{}; x < lcd::kLCDWidth; ++x) {
    // background x location
    uint8_t bg_x{static_cast<uint8_t>((scroll_x_ + x) % 256)};

    auto tile_col{bg_x / 8};

    // row major, 32 bytes per row, 32 rows, extract current map entry
    uint16_t tile_map_entry_addr{static_cast<uint16_t>(bg_tile_map_base + (tile_row * 32) + tile_col)};

    // read tile number from vram
    uint8_t tile_number{readVRAM(tile_map_entry_addr)};

    // read address
    uint16_t tile_addr{ppu::helpers::TileNumberToAddress(tile_number, is_tile_addressing_signed)};

    // address, background x and y -> shade to draw
    uint8_t shade{getShadeFromTileAddr(tile_addr, bg_y, bg_x)};

    frame_buffer_[static_cast<size_t>(ly_ * lcd::kLCDWidth) + x] =  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
        shade;
  }
}

/*
 *
 * advance
 * accepts t cycles from the cpu, and advances the dot counter
 * afance handles managing the dot counter and cycle, and orchestrates
 * corresponding functionality that comes with the advancing dot cycle
 *
 * including:
 *    Mode transitions + side effects
 *    Scanline transitions + side effects
 *
 */
void PPU::advance(uint8_t const t_cycles) {
  // guard if PPU enable is 0
  if (!ppu::helpers::IsBitSet(lcdc_, ppu::lcdc_bits::kLCDandPPUEnable)) {
    ly_ = 0;
    dot_counter_ = 0;
    mode_ = PPUMode::kHBlank;

    return;
  }

  for (uint8_t i{}; i < t_cycles; ++i) {
    ++dot_counter_;

    // for v1: because coincidence is only checked inside advanceScanline, the very first line 0
    // of the very first frame (before any wrap) doesn't get a coincidence check — but every
    // subsequent frame's ly=0 does (via the 153→0 wrap), so LYC = 0 works from frame two onward.
    // Not worth handling

    if (dot_counter_ == ppu::kDotsPerScanline) {
      advanceScanline();
    }

    PPUMode target{ppu::helpers::DeriveTargetMode(ly_, dot_counter_)};
    if (target == mode_) {
      continue;
    }

    mode_ = target;
    switch (mode_) {
      case PPUMode::kOAM:
        ppu::helpers::CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeTwoSelect, interrupt_handler_);
        break;
      case PPUMode::kDraw:
        renderScanline();
        break;
      case PPUMode::kHBlank:
        ppu::helpers::CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeZeroSelect, interrupt_handler_);
        break;
      case PPUMode::kVBlank:
        frame_complete_ = true;
        interrupt_handler_.requestInterrupt(interrupts::kVblankBit);
        ppu::helpers::CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeOneSelect, interrupt_handler_);
        break;
    }
  }
}
