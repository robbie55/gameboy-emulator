#pragma once

#include <concepts>

/*
 *
 * A contract used to ensure cartridge types have required functionality
 * while avoiding the performance implications of an abstract base class
 *
 */

template <typename Type>
concept is_cartridge_type = requires(Type rom_type, uint16_t addr, std::byte val) {
  { rom_type.read(addr) } -> std::same_as<std::byte>;
  { rom_type.write(addr, val) } -> std::same_as<void>;
};
