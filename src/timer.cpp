#include "timer.hpp"

#include <cassert>

#include "hardware_constants.hpp"

namespace {

  /*
   *
   * GetTappedBit
   *
   * Used to derive our index for div. The hardware uses a multiplexer against the div register
   * using the bottom two bits of tac as the select line.
   * 00 -> 3, 01 -> 5, 10 -> 7, 11 -> 9
   *
   * Results in an index to be used with div to check against the falling edge trigger
   * see: https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html
   *
   */
  uint8_t GetTappedBit(uint8_t tac) {
    auto bottom_two_bits{static_cast<uint8_t>(tac & timer::kTACClockSelectBits)};

    switch (bottom_two_bits) {
      case 0x00:
        return 3;
      case 0x01:
        return 5;
      case 0x10:
        return 7;
      case 0x11:
        return 9;
      default:
        assert(false && "GetTappedBit -> Derived a non 0-3 value from two bits, shouldn't happen");
    }

    return 0;
  }
}  // namespace

uint8_t Timer::readRegister(uint16_t addr) const {
  assert(addr >= io_registers::kDIV && addr <= io_registers::kTAC && "Timer::readRegister-> Given an out of bounds addr");
  switch (addr) {
    case io_registers::kDIV: {
      // mask bottom 8 bits, only bits 8-15 are read elsewhere
      return static_cast<uint8_t>((div_ & 0xFF00) >> 8);
    }
    case io_registers::kTIMA: {
      return tima_;
    }
    case io_registers::kTMA: {
      return tma_;
    }
    case io_registers::kTAC: {
      // bits 7-3 are garbage, however hardware reads them as 1, so or against 0xF8
      // see: https://gbdev.io/pandocs/Timer_and_Divider_Registers.html
      return (tac_ | 0xF8);
    }
    default:
      assert(false && "Timer::readRegister -> Reached default case in readRegsiter, invalid address in range");
  }

  return 0x00;
}

void Timer::writeRegister(uint16_t addr, uint8_t val) {
  assert(addr >= io_registers::kDIV && addr <= io_registers::kTAC && "Timer::writeRegister-> Given an out of bounds addr");
  switch (addr) {
    case io_registers::kDIV: {
      // writing any value to div resets value to 0
      // see: https://gbdev.io/pandocs/Timer_and_Divider_Registers.html

      // TODO: When implementing advance, handle falling edge race condition between 0'd out div_ and incoming increment
      div_ = 0x00;
      break;
    }
    case io_registers::kTIMA: {
      tima_ = val;
      break;
    }
    case io_registers::kTMA: {
      tma_ = val;
      break;
    }
    case io_registers::kTAC: {
      // bits 7-3 are garbage, only write to bits 2-0, leave the rest alone
      tac_ = (val & timer::kTACClockSelectBits);
      break;
    }
    default:
      assert(false && "Timer::writeRegister-> Reached default case, invalid address in range");
  }
}

void Timer::advance(uint8_t t_cycles) {
  // 1. Consume/step, per-T-cycle across the budget — increment the 16-bit div_ each tick (let it wrap naturally).
  // 2. Edge-detector input each tick: the tapped bit of div_ (selected by TAC bits 1–0) AND the enable (TAC bit 2). Your tap bits in the 16-bit model
  // are 3 / 5 / 7 / 9 for selects 01 / 10 / 11 / 00 — decide how you represent that mapping (derive from the period, or a small table).
  // 3. Fire on the 1→0 falling edge — previous input was 1, now 0 → TIMA++.
  // 4. Overflow (0xFF → 0x00): reload delay is v2, so for v1 do it immediately — TIMA = TMA, raise bit 2 (kTimerBit) into the controller.
  // 5. Factor the edge check so the DIV-write path can call it too — that's your timer.cpp:39 TODO. Zeroing div_ can drop the tapped bit 1→0; if the
  // write doesn't run the same detector, your v1 glitch silently vanishes.

  for (size_t i{}; i < t_cycles; ++i) {
    div_++;
    auto tapped_index{GetTappedBit(tac_)};
    auto enable{tac_ & timer::kTACEnableBit};
    auto signal{static_cast<uint8_t>(((div_ >> tapped_index) & 1) & enable)};
  }
}
