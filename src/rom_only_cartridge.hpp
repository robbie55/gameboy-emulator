#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

#include "cartridge_contract.hpp"
#include "cartridge_header.hpp"

class RomOnlyCartridge {
 public:
  RomOnlyCartridge(std::vector<std::byte> rom) : rom_{std::move(rom)} {
    // assert here, factory handles validation
    assert(std::size(rom_) == cartridge::kROMOnlyROMSize &&
           "RomOnlyCartridge::RomOnlyCartridge(std::vector rom) -> ROM isn't 32KiB");
  }

  RomOnlyCartridge(RomOnlyCartridge const& other) = delete;
  RomOnlyCartridge(RomOnlyCartridge&& other) = default;
  RomOnlyCartridge& operator=(RomOnlyCartridge const& other) = delete;
  RomOnlyCartridge& operator=(RomOnlyCartridge&& other) = default;

  ~RomOnlyCartridge() = default;

  [[nodiscard]] std::byte read(uint16_t addr) const;
  void write(uint16_t, std::byte);

 private:
  std::vector<std::byte> rom_;
};

static_assert(is_cartridge_type<RomOnlyCartridge>);
