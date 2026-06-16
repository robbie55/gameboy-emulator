#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "cartridge.hpp"
#include "cartridge_factory.hpp"

int main() {
  try {
    Cartridge cartridge{
        CartridgeFactory(std::vector<std::byte>{std::byte{0x1}, std::byte{0x2}, std::byte{0x3}})};
  } catch (const CartridgeException& e) {
    std::cerr << "Bad ROM: " << e.what() << '\n';
  }

  return EXIT_SUCCESS;
}
