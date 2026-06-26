#include "ppu.hpp"

#include <cassert>
#include <cstddef>

#include "hardware_constants.hpp"
#include "ppu_constants.hpp"

namespace {
  /*
   *
   * DeriveTargetMode
   *
   * accepts the ly and dot_counter member, returns a mode based on conditions set in rendering
   * see: https://gbdev.io/pandocs/Rendering.html
   *
   */
  PPUMode DeriveTargetMode(uint8_t ly, uint16_t dot_counter) {
    if (ly >= ppu::kModeOneScanlineEntry) {
      return PPUMode::kVBlank;
    }
    if (dot_counter < ppu::kModeTwoDots) {
      return PPUMode::kOAM;
    }
    if (dot_counter < (ppu::kModeTwoDots + ppu::kModeThreeDots)) {
      return PPUMode::kDraw;
    }

    return PPUMode::kHBlank;
  }

  bool IsBitSet(uint8_t reg, uint8_t bit) { return (reg & (1 << bit)) != 0; }

  /*
   *
   * CheckAndRaiseStatInterrupt
   *
   * Accepts stat and interrupt handler member, and the corresponding bit the ppu wishes to query
   * for an inerrupt if that bit is set in the stat member, raise a stat interrupt
   *
   */
  void CheckAndRaiseStatInterrupt(uint8_t stat, uint8_t bit, InterruptController& interrupt_handler) {
    if (IsBitSet(stat, bit)) {
      interrupt_handler.requestInterrupt(interrupts::kLcdStatBit);
    }
  }

  /*
   *
   * TileNumberToAddress
   *
   * Handles taking in the tile_number computed from renderScanline, and whether we are using the
   * signed or unsigned mode precendented by lcdc bit 4 (https://gbdev.io/pandocs/LCDC.html)
   *
   * Returns the calculated tile address
   *
   */
  uint16_t TileNumberToAddress(uint8_t tile_number, bool is_tile_addressing_signed) {
    uint16_t address_base{ppu::kBGWindowTileDataAreaUnsignedStart};
    uint16_t u_offset{tile_number};

    // tile address = base + offset * 16

    if (is_tile_addressing_signed) {
      address_base = ppu::kBGWindowTileDataAreaSignedStart;
      int8_t s_offset{static_cast<int8_t>(u_offset)};

      return static_cast<uint16_t>(address_base + (s_offset * 16));
    }

    return static_cast<uint16_t>(address_base + (u_offset * 16));
  }
}  // namespace

// we protect the bad addr invariant via asserts, so NOLINT serves for these accessors
uint8_t PPU::readOAM(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd && "PPU::readOAM -> Given an out of bounds addr");

  return oam_[addr];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

void PPU::writeOAM(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd && "PPU::writeOAM -> Given an out of bounds addr");

  oam_[addr] = val;  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
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

void PPU::advanceScanline() {
  dot_counter_ = 0;
  ly_++;

  if (ly_ == ppu::kScanlinesPerFrame) {
    ly_ = 0;
  }

  if (ly_ == ly_compare_) {
    CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kLYCIntSelect, interrupt_handler_);
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
  if (IsBitSet(lcdc_, ppu::lcdc_bits::kBGTileMapArea)) {
    bg_tile_map_base = ppu::kBGTileMapOneStart;
  }

  bool is_tile_addressing_signed{!IsBitSet(lcdc_, ppu::lcdc_bits::kBackgroundWindowTileDataArea)};

  // background y location
  uint8_t bg_y{static_cast<uint8_t>((scroll_y_ + ly_) % 256)};

  // current row
  auto tile_row{bg_y / 8};

  // iterate left to right on current row
  for (size_t x{}; x < lcd::kLCDWidth; ++x) {
    // background x location
    uint8_t bg_x{static_cast<uint8_t>((scroll_x_ + x) % 256)};

    // tile column
    auto tile_col{bg_x / 8};

    // row major, 32 bytes per row, 32 rows, extract current map entry
    uint16_t tile_map_entry_addr{static_cast<uint16_t>(bg_tile_map_base + (tile_row * 32) + tile_col)};

    // read tile number from vram
    uint8_t tile_number{readVRAM(tile_map_entry_addr)};

    // read address
    uint16_t tile_addr{TileNumberToAddress(tile_number, is_tile_addressing_signed)};

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
  if (!IsBitSet(lcdc_, ppu::lcdc_bits::kLCDandPPUEnable)) {
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

    PPUMode target{DeriveTargetMode(ly_, dot_counter_)};
    if (target == mode_) {
      continue;
    }

    mode_ = target;
    switch (mode_) {
      case PPUMode::kOAM:
        CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeTwoSelect, interrupt_handler_);
        break;
      case PPUMode::kDraw:
        renderScanline();
        break;
      case PPUMode::kHBlank:
        CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeZeroSelect, interrupt_handler_);
        break;
      case PPUMode::kVBlank:
        frame_complete_ = true;
        interrupt_handler_.requestInterrupt(interrupts::kVblankBit);
        CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeOneSelect, interrupt_handler_);
        break;
    }
  }
}
