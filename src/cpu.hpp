#pragma once

#include <cstdint>

namespace post_boot_state {
  inline constexpr uint16_t kAF{0x01B0};
  inline constexpr uint16_t kBC{0x0013};
  inline constexpr uint16_t kDE{0x00D8};
  inline constexpr uint16_t kHL{0x014D};
  inline constexpr uint16_t kSP{0xFFFE};
  inline constexpr uint16_t kPC{0x0100};
}  // namespace post_boot_state
