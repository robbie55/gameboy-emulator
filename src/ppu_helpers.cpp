#include "ppu_helpers.hpp"

#include "hardware_constants.hpp"
#include "ppu_constants.hpp"

namespace ppu::helpers {
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
   * Each tile is 8x8 pixels, 1 pixel is 2 bits, 128 bits, 16 bytes
   *
   * Returns the calculated tile address
   *
   */
  uint16_t TileNumberToAddress(uint8_t tile_number, bool is_tile_addressing_signed) {
    uint16_t address_base{ppu::kBGWindowTileDataAreaUnsignedStart};
    uint16_t const u_offset{tile_number};

    auto const bytes_per_tile{16};

    // tile address = base + offset * size of a tile

    if (is_tile_addressing_signed) {
      address_base = ppu::kBGWindowTileDataAreaSignedStart;
      int8_t const s_offset{static_cast<int8_t>(u_offset)};

      return static_cast<uint16_t>(address_base + (s_offset * bytes_per_tile));
    }

    return static_cast<uint16_t>(address_base + (u_offset * bytes_per_tile));
  }
}  // namespace ppu::helpers
