#pragma once

#include <cstddef>
#include <cstdint>

/*
 *
 * Serves as the location for the offsets contained in the cartridge header
 * see: https://gbdev.io/pandocs/The_Cartridge_Header.html
 *
 *
 */

namespace cartridge {
  inline constexpr uint16_t kEntryPointStart{0x0100};
  inline constexpr uint16_t kEntryPointEnd{0x0103};

  inline constexpr uint16_t kNintendoLogoStart{0x0104};
  inline constexpr uint16_t kNintendoLogoEnd{0x0133};

  inline constexpr uint16_t kTitleStart{0x0134};
  inline constexpr uint16_t kTitleEnd{0x0143};

  // future cartridge types will live along side this rom only type
  inline constexpr uint8_t kROMOnlyType{0x00};

  inline constexpr uint16_t kCartridgeType{0x0147};

  inline constexpr uint16_t kROMSize{0x0148};

  inline constexpr uint16_t kRAMSize{0x0149};

  inline constexpr uint16_t kHeaderChecksum{0x014D};

  inline constexpr uint8_t kOpenBus{0xFF};

  // 32 KiB
  inline constexpr std::size_t kROMOnlyROMSize{32768};
}  // namespace cartridge
