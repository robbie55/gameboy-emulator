#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "hardware_constants.hpp"
#include "interrupt_controller.hpp"

enum class PPUMode : uint8_t { kHBlank = 0, kVBlank = 1, kOAM = 2, kDraw = 3 };

class PPU {
 public:
  PPU() : mode_{PPUMode::kVBlank}, stat_{0x85}, lcdc_{0x91}, background_palette_{0xFC} {}

  [[nodiscard]] uint8_t readVRAM(uint16_t addr) const;
  void writeVRAM(uint16_t addr, uint8_t val);

  [[nodiscard]] uint8_t readOAM(uint16_t addr) const;
  void writeOAM(uint16_t addr, uint8_t val);

 private:
  void setStatAndMode(uint8_t stat, PPUMode mode);

  // inclusive, +1 to prevent off by one errors
  std::array<uint8_t,
             static_cast<std::size_t>(game_boy_memory::kVRAMEnd - game_boy_memory::kVRAMStart + 1)>
      vram_{};
  std::array<uint8_t,
             static_cast<std::size_t>(game_boy_memory::kOAMEnd - game_boy_memory::kOAMStart + 1)>
      oam_{};

  std::array<uint8_t, static_cast<std::size_t>(lcd::kLCDHeight* lcd::kLCDWidth)> frame_buffer_{};

  uint16_t dot_counter_{};

  // these two are directly related, protect with an accessor + assert to ensure the invariant
  // between bits 0-1 and mode are protected
  PPUMode mode_{};
  uint8_t stat_{};

  uint8_t ly_{};
  uint8_t ly_compare_{};
  uint8_t lcdc_{};
  uint8_t scroll_x_{};
  uint8_t scroll_y_{};
  uint8_t window_x_{};
  uint8_t window_y_{};
  uint8_t background_palette_{};
  uint8_t obj_sprite_palette_zero_{};
  uint8_t obj_sprite_palette_one_{};

  InterruptController interrupt_handle_;
};
