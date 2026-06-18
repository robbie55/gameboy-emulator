#include "ppu.hpp"

#include <cassert>

#include "hardware_constants.hpp"

// we protect the bad addr invariant via asserts, so NOLINT serves for these accessors
uint8_t PPU::readOAM(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd &&
         "PPU::readOAM -> Given an out of bounds addr");

  return oam_[addr];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

void PPU::writeOAM(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kOAMStart && addr <= game_boy_memory::kOAMEnd &&
         "PPU::writeOAM -> Given an out of bounds addr");

  oam_[addr] = val;  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

uint8_t PPU::readVRAM(const uint16_t addr) const {
  assert(addr >= game_boy_memory::kVRAMStart && addr <= game_boy_memory::kVRAMEnd &&
         "PPU::readVRAM -> Given an out of bounds addr");

  return vram_[addr];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

void PPU::writeVRAM(const uint16_t addr, const uint8_t val) {
  assert(addr >= game_boy_memory::kVRAMStart && addr <= game_boy_memory::kVRAMEnd &&
         "PPU::writeVRAM -> Given an out of bounds addr");

  vram_[addr] = val;  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index
}

void PPU::setStatAndMode(uint8_t stat, PPUMode mode) {
  assert(static_cast<PPUMode>(stat & 0x03) == mode &&
         "PPU::setStatAndMode -> stat's bottom two bits don't align with mode");

  stat_ = stat;
  mode_ = mode;
}
