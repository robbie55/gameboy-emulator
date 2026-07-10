#pragma once

#include <cstdint>

#include "interrupt_controller.hpp"
class Timer {
 public:
  explicit Timer(InterruptController& interrupt_handler) : interrupt_handler_{interrupt_handler} {}

  Timer(Timer const& rhs) = delete;
  Timer& operator=(Timer const& rhs) = delete;
  Timer(Timer&& rhs) = delete;
  Timer& operator=(Timer&& rhs) = delete;

  ~Timer() = default;

  [[nodiscard]] uint8_t readRegister(uint16_t addr) const;
  void writeRegister(uint16_t addr, uint8_t val);

  void advance(uint8_t t_cycles);

 private:
  uint16_t div_{0xABCC};
  uint8_t tima_{};
  uint8_t tma_{};
  // docs have non-read bits set to 1, as hardware reads these 'garbage' values as 1
  // see: https://gbdev.io/pandocs/Power_Up_Sequence.html?highlight=TAC#hardware-registers
  uint8_t tac_{0xF8};

  // non owning relationship, OK to use a ref here
  InterruptController& interrupt_handler_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
};
