#pragma once

#include <cstdint>

namespace joypad {
  inline constexpr uint8_t kSelectButtonBit{5};
  inline constexpr uint8_t kSelectDPadBit{4};

  inline constexpr uint8_t kButtonPressed{0};

  // masks used for writes, mask all bits w/0 except corresponding select bit
  inline constexpr uint8_t kSelectButtonMask{0x20};
  inline constexpr uint8_t kSelectDPadMask{0x10};
};  // namespace joypad
