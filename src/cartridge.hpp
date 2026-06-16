#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>

#include "cartridge_contract.hpp"
#include "rom_only_cartridge.hpp"

/*
 *
 * Cartridge Wrapper class serves as an abtraction between the actual cartridge type (Rom only,
 * MCB1, MBC3, MBC5) And the other modules which will interact with the cartridge. We use strategy
 * to inject the cartridge type functionality, the only physical coupling of the three types live
 * here. For v1, only RomOnlyCartridge exists.
 *
 *
 */

class Cartridge {
 private:
  using CartridgeStrategy = std::variant<RomOnlyCartridge>;

 public:
  Cartridge(CartridgeStrategy cartridge) : cartridge_(std::move(cartridge)) {};
  Cartridge(Cartridge const& other) = delete;
  Cartridge(Cartridge&& other) = default;

  Cartridge& operator=(Cartridge const& other) = delete;
  Cartridge& operator=(Cartridge&& other) = default;

  ~Cartridge() = default;

  [[nodiscard]] std::byte read(uint16_t addr) const {
    return std::visit([addr](const RomOnlyCartridge& c) -> std::byte { return c.read(addr); },
                      cartridge_);
  };
  void write(uint16_t addr, std::byte const val) {
    std::visit([addr, &val](RomOnlyCartridge& c) -> void { c.write(addr, val); }, cartridge_);
  }

 private:
  CartridgeStrategy cartridge_;
};

static_assert(is_cartridge_type<Cartridge>);
