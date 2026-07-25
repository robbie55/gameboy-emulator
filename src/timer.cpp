#include "timer.hpp"

#include <cassert>
#include <cstddef>

#include "hardware_constants.hpp"

namespace {

  /*
   *
   * GetTappedBit
   *
   * Used to derive our index for div. The hardware uses a multiplexer against the div register
   * using the bottom two bits of tac as the select line.
   * 00 -> 9, 01 -> 3, 10 -> 5, 11 -> 7
   *
   * These indices correspond to a bit in div which determines which clock period size to use
   *
   * Results in an index to be used with div to check against the falling edge trigger
   * see: https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html
   *
   */
  uint8_t GetTappedBit(uint8_t tac) {
    auto bottom_two_bits{static_cast<uint8_t>(tac & timer::kTACClockSelectBits)};

    switch (bottom_two_bits) {
      case 0:
        return 9;
      case 1:
        return 3;
      case 2:
        return 5;
      case 3:
        return 7;
      default:
        assert(false && "GetTappedBit -> Derived a non 0-3 value from two bits, shouldn't happen");
    }

    return 0;
  }
}  // namespace

void Timer::handleTIMAOverflow() {
  // if tima is set to 0x00 post increment, it's overflowed
  // set tima to tma, and request an interrupt
  if (tima_ == 0x00) {
    tima_ = tma_;
    interrupt_controller_.requestInterrupt(interrupts::kTimerBit);
  }
}

/*
 *
 * detectFallingEdge
 *
 * Handles checking if we triggered the falling edge hardware. This function SHOULD
 * be called only when advancing our timer, however due to the way tac_ and div_ are
 * written to, it can unintentionally be triggered when simply writing to the registers.
 * This is a notorious glitch within the gameboy, so we emulate it to stay faithful to the hardware
 *
 */
void Timer::detectFallingEdge() {
  auto const tapped_index{GetTappedBit(tac_)};
  auto const enable{(tac_ >> timer::kTACEnableBit) & 1};
  auto const signal{static_cast<bool>(((div_ >> tapped_index) & 1) & enable)};

  // on falling edge, increment TIMA
  if (prev_signal_ && !signal) {
    tima_++;

    handleTIMAOverflow();
  }

  prev_signal_ = signal;
}

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

      div_ = 0x00;
      detectFallingEdge();
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
      tac_ = (val & (timer::kTACClockSelectBits | 1 << timer::kTACEnableBit));
      detectFallingEdge();
      break;
    }
    default:
      assert(false && "Timer::writeRegister-> Reached default case, invalid address in range");
  }
}

void Timer::advance(uint8_t t_cycles) {
  for (std::size_t i{}; i < t_cycles; ++i) {
    div_++;
    detectFallingEdge();
  }
}
