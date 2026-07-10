#pragma once

#include <cstdint>

#include "interrupt_controller.hpp"
#include "ppu_mode.hpp"

namespace ppu::helpers {
  PPUMode DeriveTargetMode(uint8_t ly, uint16_t dot_counter);

  bool IsBitSet(uint8_t reg, uint8_t bit);

  void CheckAndRaiseStatInterrupt(uint8_t stat, uint8_t bit, InterruptController& interrupt_handler);

  uint16_t TileNumberToAddress(uint8_t tile_number, bool is_tile_addressing_signed);
}  // namespace ppu::helpers
