#pragma once

#include <cstdint>

namespace joypad {
  inline constexpr uint8_t kSelectButtonBit{5};
  inline constexpr uint8_t kSelectDPadBit{4};
  inline constexpr uint8_t kStartDownBit{3};
  inline constexpr uint8_t kSelectUpBit{2};
  inline constexpr uint8_t kBLeftBit{1};
  inline constexpr uint8_t kARightBit{0};

  inline constexpr uint8_t kButtonPressed{0};
};  // namespace joypad
