#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "cartridge.hpp"

class CartridgeException : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

Cartridge CartridgeFactory(std::vector<std::byte> rom);
