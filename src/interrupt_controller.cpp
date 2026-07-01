#include "interrupt_controller.hpp"

#include <bit>
#include <cassert>
#include <cstddef>

// hardware won't ever set the top three bits (5, 6 ,7), so we assert against them in requestInterrupt and clearFlagBit
// Here, we mask against them, query should not raise asserts
// see: https://gbdev.io/pandocs/Interrupts.html - https://gbdev.io/pandocs/Interrupt_Sources.html

std::optional<int> InterruptController::queryPendingInterrupt() const {
  std::byte pending_mask{interrupt_enable_ & interrupt_flag_ & std::byte{0x1F}};

  if (pending_mask == std::byte{0}) {
    return {};
  }

  // countr_zero will count trailing 0's, 1100 -> yields 2
  return std::countr_zero(std::to_integer<uint8_t>(pending_mask));
}

//
void InterruptController::requestInterrupt(uint8_t bit) {
  assert(bit < 5 &&
         "InterruptController::requestInterrupt-> bit with value greater than 4 requested an "
         "interrupt");
  interrupt_flag_ |= (std::byte{1} << bit);
}

void InterruptController::clearFlagBit(uint8_t bit) {
  assert(bit < 5 && "InterruptController::clearFlagBit -> bit with value greater than 4 passed to clear");
  interrupt_flag_ &= ~(std::byte{1} << bit);
}
