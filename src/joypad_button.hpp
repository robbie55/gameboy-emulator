#pragma once

#include <cstdint>
#include <type_traits>

// enum used as both identifiers for button conditionals and bit positions of buttons in Joypad::joypad_buttons_
namespace joypad {
  enum class Button : uint8_t { kRight = 0, kLeft, kUp, kDown, kA, kB, kSelect, kStart };

  constexpr uint8_t operator<<(int lhs, Button rhs) { return static_cast<uint8_t>(lhs << static_cast<uint8_t>(rhs)); }
}  // namespace joypad
