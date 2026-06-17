#include <cstddef>

#include "doctest.h"
#include "interrupt_controller.hpp"

TEST_CASE("Interrupt Controller Query") {
  SUBCASE("Query Raises an Interrupt if Enabled and Flagged") {
    InterruptController controller{};
    controller.setEnable(std::byte{0x01});
    controller.setFlag(std::byte{0x00});

    // returns nullopt via optional
    CHECK_FALSE(controller.queryPendingInterrupt());

    controller.setEnable(std::byte{0x00});
    controller.setFlag(std::byte{0x01});

    // returns nullopt via optional
    CHECK_FALSE(controller.queryPendingInterrupt());

    controller.setEnable(std::byte{0x02});
    controller.setFlag(std::byte{0x02});

    // both enable and flag set, should return std::optional<0> for 1st bit
    CHECK_EQ(controller.queryPendingInterrupt().value(), 1);
  }

  SUBCASE("Query Correctly Returns most Prioritized Interrupt") {
    InterruptController controller{};

    // bit 1 and 0 both set
    controller.setEnable(std::byte{0x03});
    controller.setFlag(std::byte{0x03});

    // returns lowest bit, 0
    CHECK_EQ(controller.queryPendingInterrupt().value(), 0);

    // bit 4 and 2 both set
    controller.setEnable(std::byte{0x14});
    controller.setFlag(std::byte{0x14});

    // returns lowest bit, 2
    CHECK_EQ(controller.queryPendingInterrupt().value(), 2);
  }

  SUBCASE("Query doesn't trigger on top 3 bits") {
    InterruptController controller{};

    // all bits enabled
    controller.setEnable(std::byte{0xFF});
    // top 3 bits flagged, rest are 0
    controller.setFlag(std::byte{0xE0});

    // expect a nullopt from std::optional
    CHECK_FALSE(controller.queryPendingInterrupt());
  }

  SUBCASE("Query Round Trip Behavior") {
    InterruptController controller{};

    // bit 1 and 0 both set
    controller.setEnable(std::byte{0x03});
    controller.setFlag(std::byte{0x03});

    int bit_zero{controller.queryPendingInterrupt().value()};

    CHECK_EQ(bit_zero, 0);

    controller.clearFlagBit(bit_zero);

    int bit_one{controller.queryPendingInterrupt().value()};

    CHECK_EQ(bit_one, 1);

    controller.clearFlagBit(bit_one);

    // expecting a nullopt via std::optional since nothing set
    CHECK_FALSE(controller.queryPendingInterrupt());
  }
}

TEST_CASE("Interrupt Controller State Management") {
  SUBCASE("Initializes Registers Correctly") {
    InterruptController controller{};

    CHECK_EQ(controller.getEnable(), std::byte{0x00});
    CHECK_EQ(controller.getFlag(), std::byte{0xE1});
  }

  SUBCASE("Request Interrupt is Non-Destructive") {
    InterruptController controller{};

    // set bit 1
    controller.requestInterrupt(1);

    std::byte interrupt_flag{controller.getFlag()};

    // bit 1 should be set, equal to two
    CHECK_EQ(std::to_integer<int>(interrupt_flag & std::byte{0x02}), 2);

    // set bit 3
    controller.requestInterrupt(3);
    interrupt_flag = controller.getFlag();

    // bit 3 and bit 1 set,
    CHECK_EQ(std::to_integer<int>(interrupt_flag & std::byte{0x0A}), 10);
  }
}
