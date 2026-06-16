#include "cartridge_factory.hpp"

#include <cstddef>
#include <iterator>
#include <utility>

#include "cartridge_header.hpp"
#include "rom_only_cartridge.hpp"

/*
 *
 * Cartridge Factory
 * Responsible for returning a valid Cartridge or fail cleanly.
 * For v1, the only variant supported is the ROM Only (No MBC) type
 *
 */

Cartridge CartridgeFactory(std::vector<std::byte> rom) {
  // ROM header lives at 0x150
  if (std::size(rom) < 0x150) {
    throw CartridgeException("ROM is truncated, can't read ROM header");
  }
  if (std::to_integer<uint8_t>(rom[cartridge::kCartridgeType]) != cartridge::kROMOnlyType) {
    throw CartridgeException("ROM Cartridge Type doesn't match ROM Only Expected Type");
  }
  if (std::size(rom) != cartridge::kROMOnlyROMSize) {
    throw CartridgeException("ROM size isn't 32KiB");
  }

  return Cartridge{RomOnlyCartridge{std::move(rom)}};
}
