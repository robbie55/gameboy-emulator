#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "hardware_constants.hpp"
#include "interrupt_controller.hpp"
#include "ppu_mode.hpp"

class PPU {
 private:
  void renderScanline();
  void advanceScanline();
  [[nodiscard]] uint8_t getShadeFromTileAddr(uint16_t tile_addr, uint8_t bg_y, uint8_t bg_x) const;

 public:
  explicit PPU(InterruptController& interrupt_handler)
      : mode_{PPUMode::kOAM}, lcdc_{0x91}, background_palette_{0xFC}, interrupt_handler_{interrupt_handler} {}

  // no moving or copying, ref& member variable
  PPU() = delete;
  PPU(PPU const& other) = delete;
  PPU(PPU&& other) = delete;
  PPU& operator=(PPU const& other) = delete;
  PPU& operator=(PPU&& other) = delete;
  ~PPU() = default;

  [[nodiscard]] uint8_t readVRAM(uint16_t addr) const;
  void writeVRAM(uint16_t addr, uint8_t val);

  [[nodiscard]] uint8_t readOAM(uint16_t addr) const;
  void writeOAM(uint16_t addr, uint8_t val);

  [[nodiscard]] bool isFrameComplete() const { return frame_complete_; }
  void clearFrameComplete() { frame_complete_ = false; }

  void advance(uint8_t t_cycles);

 private:
  // inclusive, +1 to prevent off by one errors
  std::array<uint8_t, static_cast<std::size_t>(game_boy_memory::kVRAMEnd - game_boy_memory::kVRAMStart + 1)> vram_{};
  std::array<uint8_t, static_cast<std::size_t>(game_boy_memory::kOAMEnd - game_boy_memory::kOAMStart + 1)> oam_{};

  std::array<uint8_t, static_cast<std::size_t>(lcd::kLCDHeight* lcd::kLCDWidth)> frame_buffer_{};

  uint16_t dot_counter_{};

  // store a mode to derive the bottom two bits of a stat register, stat_int_select_ only stores
  // bits 3-6, exclude mode, coincidence and 7th bit
  PPUMode mode_{};
  uint8_t stat_int_select_{};

  bool frame_complete_{};

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

  // non owning relationship, OK to use a ref here
  InterruptController& interrupt_handler_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
};
