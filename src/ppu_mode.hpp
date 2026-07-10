#pragma once

#include <cstdint>

enum class PPUMode : uint8_t { kHBlank = 0, kVBlank = 1, kOAM = 2, kDraw = 3 };
