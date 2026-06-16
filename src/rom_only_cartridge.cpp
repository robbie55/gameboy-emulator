#include "rom_only_cartridge.hpp"

#include "cartridge_header.hpp"
#include "hardware_constants.hpp"

std::byte RomOnlyCartridge::read(uint16_t addr) const {
  if (addr <= game_boy_memory::kROMBankSwitchEnd) {
    return rom_[addr];
  }

  return static_cast<std::byte>(cartridge::kOpenBus);
}

void RomOnlyCartridge::write(uint16_t, std::byte) {}
