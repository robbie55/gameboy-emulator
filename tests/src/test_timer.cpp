#include <doctest.h>

#include <cstddef>
#include <limits>

#include "hardware_constants.hpp"
#include "interrupt_controller.hpp"
#include "timer.hpp"

namespace {
  void AdvanceTimerByTCycles(Timer& timer, auto t_cycles) {
    // t_cycles are 8 bit, if we want to increment beyond that, compare against max
    uint16_t const max_t_cycles{std::numeric_limits<uint8_t>::max()};

    while (t_cycles) {
      const auto iter_t_cycles{static_cast<uint8_t>(t_cycles > max_t_cycles ? max_t_cycles : t_cycles)};

      timer.advance(iter_t_cycles);

      t_cycles -= iter_t_cycles;
    }
  }

  void SeedForTIMAIncrement(Timer& timer, uint8_t bottom_two_bits) {
    // zero out DIV register
    timer.writeRegister(io_registers::kDIV, 0x00);

    // enable tac signal enable bit, set bottom two bits
    auto tac{timer.readRegister(io_registers::kTAC)};
    tac |= (1 << timer::kTACEnableBit | bottom_two_bits);
    timer.writeRegister(io_registers::kTAC, tac);
  }
}  // namespace

TEST_CASE("Register Accessors") {
  SUBCASE("Round Trip Access") {
    InterruptController controller{};
    Timer timer{controller};

    // don't include tac or div, special write rules
    std::vector addrs{io_registers::kTMA, io_registers::kTIMA};

    for (std::size_t i{}; i < std::size(addrs); ++i) {
      auto const cur_addr{addrs.at(i)};
      timer.writeRegister(cur_addr, i);

      CHECK(timer.readRegister(cur_addr) == i);
    }

    // for tac, bits 3-7 are garbage, and masked on write
    // when read back, hardware returns 1 for those garbage bits,
    // so write a value which only writes to the first 3 bits
    timer.writeRegister(io_registers::kTAC, 0x07);

    // assert that first three bits are 1, and the rest are 1, proving mask
    CHECK(timer.readRegister(io_registers::kTAC) == 0xFF);
  }
}

TEST_CASE("Timer Advance") {
  SUBCASE("DIV Register Increments on advance") {
    InterruptController controller{};
    Timer timer{controller};

    // div initialized at ABCC, force it to 0 by writing
    timer.writeRegister(io_registers::kDIV, 0x00);

    CHECK(timer.readRegister(io_registers::kDIV) == 0);

    // bottom 8 bits of div are masked, and register is returned as 8 bit, so increment to a value that involves upper 8 bits
    AdvanceTimerByTCycles(timer, 0xFFFF);

    // assert against upper 8 bits of register
    CHECK(timer.readRegister(io_registers::kDIV) == 0xFF);

    // assert writes to div 0 the reg out
    timer.writeRegister(io_registers::kDIV, 0xFF);
    CHECK(timer.readRegister(io_registers::kDIV) == 0x00);

    // advance back to FF, then advance once more to check for overflow
    AdvanceTimerByTCycles(timer, 0xFFFF);
    CHECK(timer.readRegister(io_registers::kDIV) == 0xFF);

    timer.advance(1);
    CHECK(timer.readRegister(io_registers::kDIV) == 0x00);
  }

  SUBCASE("TIMA Increments on enable, TAC select 00") {
    InterruptController controller{};
    Timer timer{controller};

    auto const bottom_two_bits{0};
    SeedForTIMAIncrement(timer, bottom_two_bits);

    // first two bits of tac should be 0x00, so tapped index is 9, see advance() and detectFallingEdge()
    // advance to a point where div_ is one iteration away from having an on 9th bit
    AdvanceTimerByTCycles(timer, 0x01FF);

    // advance once more, 9th bit is set, tapped index is 9, enable bit on tac is set, signal is now on
    timer.advance(1);

    // assert tima is 0
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);

    // increment until 9th bit is turned off, and assert falling edge trigger
    AdvanceTimerByTCycles(timer, 0x04FF);

    CHECK(timer.readRegister(io_registers::kTIMA) == 1);
  }

  SUBCASE("TIMA Increments on enable, TAC select 01") {
    InterruptController controller{};
    Timer timer{controller};

    auto const bottom_two_bits{1};
    SeedForTIMAIncrement(timer, bottom_two_bits);

    // first two bits of tac should be 01, so tapped index is 3, see advance() and detectFallingEdge()
    // advance to a point where div_ is one iteration away from having an on 3th bit
    AdvanceTimerByTCycles(timer, 0x0007);

    // advance once more, 9th bit is set, tapped index is 3, enable bit on tac is set, signal is now on
    timer.advance(1);

    // assert tima is 0
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);

    // increment until 3rd bit is turned off, and assert falling edge trigger
    AdvanceTimerByTCycles(timer, 0x0010);

    CHECK(timer.readRegister(io_registers::kTIMA) == 1);
  }

  SUBCASE("TIMA Increments on enable, TAC select 10") {
    InterruptController controller{};
    Timer timer{controller};

    auto const bottom_two_bits{2};
    SeedForTIMAIncrement(timer, bottom_two_bits);

    // first two bits of tac should be 10, so tapped index is 5, see advance() and detectFallingEdge()
    // advance to a point where div_ is one iteration away from having an on 5th bit
    AdvanceTimerByTCycles(timer, 0x001F);

    // advance once more, 5th bit is set, tapped index is 5, enable bit on tac is set, signal is now on
    timer.advance(1);

    // assert tima is 0
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);

    // increment until 5th bit is turned off, and assert falling edge trigger
    AdvanceTimerByTCycles(timer, 0x0040);

    CHECK(timer.readRegister(io_registers::kTIMA) == 1);
  }

  SUBCASE("TIMA Increments on enable, TAC select 11") {
    InterruptController controller{};
    Timer timer{controller};

    auto const bottom_two_bits{3};
    SeedForTIMAIncrement(timer, bottom_two_bits);

    // first two bits of tac should be 11, so tapped index is 7, see advance() and detectFallingEdge()
    // advance to a point where div_ is one iteration away from having an on 7th bit
    AdvanceTimerByTCycles(timer, 0x007F);

    // advance once more, 7th bit is set, tapped index is 7, enable bit on tac is set, signal is now on
    timer.advance(1);

    // assert tima is 0
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);

    // increment until 7th bit is turned off, and assert falling edge trigger
    AdvanceTimerByTCycles(timer, 0x0100);

    CHECK(timer.readRegister(io_registers::kTIMA) == 1);
  }

  SUBCASE("Full TIMA Increment cycle, no enable, doesn't increment TIMA") {
    InterruptController controller{};
    Timer timer{controller};

    // zero out div
    timer.writeRegister(io_registers::kDIV, 0x00);

    // seed for bottom two being 10, but disable tac
    auto const bottom_two_bits{2};

    auto tac{timer.readRegister(io_registers::kTAC)};
    tac |= bottom_two_bits;
    timer.writeRegister(io_registers::kTAC, tac);

    // bottom two = 10, tapped index should be 5, advance to right before 5th bit is set
    AdvanceTimerByTCycles(timer, 0x001F);

    // advance once more, 5th bit is set, tapped index is 5, enable bit on tac is set, signal is now on
    timer.advance(1);

    // assert tima is 0
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);

    // increment until 5th bit is turned off, and assert falling edge trigger
    AdvanceTimerByTCycles(timer, 0x0040);

    // assert tima didn't increment since we never enabled tac
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);
  }

  SUBCASE("TIMA Overflow, set to TMA") {
    InterruptController controller{};
    Timer timer{controller};

    // seed for bottom two is 10, 5th bit index
    const auto bottom_two_bits{2};
    SeedForTIMAIncrement(timer, bottom_two_bits);

    // set tima reg to max value, 0xFF
    timer.writeRegister(io_registers::kTIMA, 0xFF);

    // write arbitrary testing value to TMA
    const auto tma_value{0xAB};
    timer.writeRegister(io_registers::kTMA, tma_value);

    // advance to signal on
    AdvanceTimerByTCycles(timer, 0x001F);
    timer.advance(1);

    // check tima before increment
    CHECK(timer.readRegister(io_registers::kTIMA) == 0xFF);

    // TIMA overflow requsts an interrupt, query before
    auto interrupt_flag{controller.getFlag()};
    CHECK_FALSE((std::to_integer<uint8_t>(interrupt_flag >> interrupts::kTimerBit) & 1));

    // increment until 5th bit is turned off, and assert falling edge trigger
    AdvanceTimerByTCycles(timer, 0x0040);

    // rather than overflow wrap, 0xFF -> 0x00, TIMA takes TMA value
    CHECK(timer.readRegister(io_registers::kTIMA) == tma_value);

    // ensure interrupt is raised
    interrupt_flag = controller.getFlag();
    CHECK((std::to_integer<uint8_t>(interrupt_flag >> interrupts::kTimerBit) & 1));
  }

  SUBCASE("DIV Write triggers falling edge glitch") {
    InterruptController controller{};
    Timer timer{controller};

    // seed for bottom two is 10, 5th bit index
    const auto bottom_two_bits{2};
    SeedForTIMAIncrement(timer, bottom_two_bits);

    // advance to signal on
    AdvanceTimerByTCycles(timer, 0x001F);
    timer.advance(1);

    // check tima before increment
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);

    // rather than advance until 5th bit is off, write to div directly
    timer.writeRegister(io_registers::kDIV, 0);

    // assert tima still incremented, with div 0'd out
    CHECK(timer.readRegister(io_registers::kTIMA) == 1);
  }

  SUBCASE("TAC Write triggers falling edge glitch") {
    InterruptController controller{};
    Timer timer{controller};

    // seed for bottom two is 10, 5th bit index
    const auto bottom_two_bits{2};
    SeedForTIMAIncrement(timer, bottom_two_bits);

    // advance to signal on
    AdvanceTimerByTCycles(timer, 0x001F);
    timer.advance(1);

    // check tima before increment
    CHECK(timer.readRegister(io_registers::kTIMA) == 0);

    // rather than advance until 5th bit is off, write to tac directly, without enable turned on
    auto tac{timer.readRegister(io_registers::kTAC)};
    tac &= ~(1 << timer::kTACEnableBit);
    timer.writeRegister(io_registers::kTAC, tac);

    // assert tima still incremented, with tac enable turned off
    CHECK(timer.readRegister(io_registers::kTIMA) == 1);
  }
}
