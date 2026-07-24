#pragma once

#include <cstdint>

// enum used as both identifiers for button conditionals and bit positions of buttons in Joypad::joypad_buttons_
namespace joypad {
  enum class Button : uint8_t { kRight = 0, kLeft, kUp, kDown, kA, kB, kSelect, kStart };
}
