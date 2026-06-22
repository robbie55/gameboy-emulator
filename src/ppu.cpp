#include "ppu.hpp"

#include <cassert>

#include "hardware_constants.hpp"
#include "ppu_constants.hpp"

namespace {
  /*
   *
   * DeriveTargetMode
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
   * Accepts stat and interrupt handler member, and the corresponding bit the ppu wishes to query
   * for an inerrupt if that bit is set in the stat member, raise a stat interrupt
   *
   */
  void CheckAndRaiseStatInterrupt(uint8_t stat, uint8_t bit,
                                  InterruptController& interrupt_handler) {
    if (IsBitSet(stat, bit)) {
      interrupt_handler.requestInterrupt(interrupts::kLcdStatBit);
    }
  }
}  // namespace

// we protect the bad addr invariant via asserts, so NOLINT serves for these accessors
uint8_t PPU::readOAM(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd &&
         "PPU::readOAM -> Given an out of bounds addr");

  return oam_[addr];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

void PPU::writeOAM(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd &&
         "PPU::writeOAM -> Given an out of bounds addr");

  oam_[addr] = val;  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

uint8_t PPU::readVRAM(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kVRAMStart && addr <= game_boy_memory::kVRAMEnd &&
         "PPU::readVRAM -> Given an out of bounds addr");

  return vram_[addr];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

void PPU::writeVRAM(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kVRAMStart && addr <= game_boy_memory::kVRAMEnd &&
         "PPU::writeVRAM -> Given an out of bounds addr");

  vram_[addr] = val;  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
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
        CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeTwoSelect,
                                   interrupt_handler_);
        break;
      case PPUMode::kDraw:
        renderScanline();
        break;
      case PPUMode::kHBlank:
        CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeZeroSelect,
                                   interrupt_handler_);
        break;
      case PPUMode::kVBlank:
        frame_complete_ = true;
        interrupt_handler_.requestInterrupt(interrupts::kVblankBit);
        CheckAndRaiseStatInterrupt(stat_int_select_, ppu::stat_bits::kModeOneSelect,
                                   interrupt_handler_);
        break;
    }
  }
}
