#pragma once

#include <cstdint>

namespace ppu {
  inline constexpr uint16_t kDotsPerScanline{456};

  inline constexpr uint8_t kModeZeroDots{151};
  inline constexpr uint8_t kModeTwoDots{80};
  inline constexpr uint8_t kModeThreeDots{225};

  static_assert(kModeZeroDots + kModeTwoDots + kModeThreeDots == kDotsPerScanline);

  inline constexpr uint8_t kModeOneScanlineEntry{144};

  inline constexpr uint16_t kScanlinesPerFrame{154};

  inline constexpr uint32_t kDotsPerFrame{kScanlinesPerFrame * kDotsPerScanline};

  inline constexpr uint16_t kBGTileMapZeroStart{0x9800};
  inline constexpr uint16_t kBGTileMapOneStart{0x9C00};

  inline constexpr uint16_t kBGWindowTileDataAreaUnsignedStart{0x8000};
  inline constexpr uint16_t kBGWindowTileDataAreaSignedStart{0x9000};
}  // namespace ppu

namespace ppu::lcdc_bits {
  inline constexpr uint8_t kLCDandPPUEnable{7};
  inline constexpr uint8_t kWindowTileMapArea{6};
  inline constexpr uint8_t kWindowEnable{5};
  inline constexpr uint8_t kBackgroundWindowTileDataArea{4};
  inline constexpr uint8_t kBGTileMapArea{3};
  inline constexpr uint8_t kObjSize{2};
  inline constexpr uint8_t kObjEnable{1};
  inline constexpr uint8_t kBackgroundWindowEnablePriority{0};
}  // namespace ppu::lcdc_bits

namespace ppu::stat_bits {
  inline constexpr uint8_t kLYCIntSelect{6};
  inline constexpr uint8_t kModeTwoSelect{5};
  inline constexpr uint8_t kModeOneSelect{4};
  inline constexpr uint8_t kModeZeroSelect{3};
  inline constexpr uint8_t kLYCEqualsLYFlag{2};
  inline constexpr uint8_t kPPUModeEnd{1};
  inline constexpr uint8_t kPPUModeBegin{0};

  inline constexpr uint8_t kStatWriteableMask{1 << kLYCIntSelect | 1 << kModeTwoSelect | 1 << kModeOneSelect | 1 << kModeZeroSelect};
}  // namespace ppu::stat_bits
