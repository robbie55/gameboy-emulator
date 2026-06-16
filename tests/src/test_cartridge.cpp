#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "cartridge.hpp"
#include "cartridge_factory.hpp"
#include "cartridge_header.hpp"
#include "rom_only_cartridge.hpp"

TEST_CASE("Rom Only Cartridge") {
  SUBCASE("Out of Bounds read return kOpenBus") {
    std::vector<std::byte> dummy_rom(32768, std::byte{0x00});

    Cartridge cartridge{RomOnlyCartridge{std::move(dummy_rom)}};

    CHECK(std::to_integer<uint8_t>(cartridge.read(0xA000)) == cartridge::kOpenBus);
  }

  SUBCASE("Rom Only Cartridge Inbounds Read") {
    std::vector<std::byte> dummy_rom(32768, std::byte{0x00});

    constexpr std::byte kDummyRead{0x32};
    constexpr uint16_t kDummyReadAddr{0x0200};

    dummy_rom.at(kDummyReadAddr) = kDummyRead;
    Cartridge cartridge{RomOnlyCartridge{std::move(dummy_rom)}};

    CHECK(cartridge.read(kDummyReadAddr) == kDummyRead);
  }

  SUBCASE("Rom Only Cartridge Write voids") {
    std::vector<std::byte> dummy_rom(32768, std::byte{0x00});

    constexpr std::byte kDummyVal{0x32};
    constexpr uint16_t kDummyAddr{0x0200};

    Cartridge cartridge{RomOnlyCartridge{std::move(dummy_rom)}};

    // write should void, so we check the value is still 0x00 after write
    cartridge.write(kDummyAddr, kDummyVal);

    CHECK(cartridge.read(kDummyAddr) == std::byte{0x00});
  }
}

TEST_CASE("Cartridge Factory") {
  SUBCASE("Rom Only Construction: Happy Path") {
    std::vector<std::byte> dummy_rom(32768, std::byte{0x00});

    dummy_rom.at(cartridge::kCartridgeType) = static_cast<std::byte>(cartridge::kROMOnlyType);

    constexpr std::byte kDummyRead{0x15};
    constexpr uint16_t kDummyReadAddr{0x0100};

    dummy_rom.at(kDummyReadAddr) = kDummyRead;

    Cartridge cartridge{CartridgeFactory(dummy_rom)};

    CHECK(cartridge.read(kDummyReadAddr) == kDummyRead);
  }

  SUBCASE("Rom Only Construction: Truncated Rom Header") {
    // truncated ROM with a missing header, will throw
    std::vector<std::byte> dummy_rom(300, std::byte{0x00});

    CHECK_THROWS_AS(CartridgeFactory(dummy_rom), CartridgeException);
  }

  SUBCASE("Rom Only Construction: Invalid Rom Size") {
    // truncated ROM, not 32KiB
    std::vector<std::byte> dummy_rom(32760, std::byte{0x00});

    dummy_rom.at(cartridge::kCartridgeType) = static_cast<std::byte>(cartridge::kROMOnlyType);

    CHECK_THROWS_AS(CartridgeFactory(dummy_rom), CartridgeException);
  }

  SUBCASE("Rom Only Construction: Wrong type in ROM Header") {
    std::vector<std::byte> dummy_rom(32768, std::byte{0x00});

    dummy_rom.at(cartridge::kCartridgeType) = std::byte{0x01};

    CHECK_THROWS_AS(CartridgeFactory(dummy_rom), CartridgeException);
  }
}
