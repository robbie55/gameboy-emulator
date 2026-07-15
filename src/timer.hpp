#pragma once

#include <cstdint>

#include "interrupt_controller.hpp"
class Timer {
 private:
  void handleTIMAOverflow();
  void detectFallingEdge();

 public:
  explicit Timer(InterruptController& interrupt_controller) : interrupt_controller_{interrupt_controller} {}

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
  uint8_t tac_{};

  // prev_signal_ should start consistent with your post-boot div_/tac_ so the first tick doesn't fabricate an edge.
  // Post-boot the timer's disabled (enable=0 → signal=0)
  bool prev_signal_{};

  // non owning relationship, OK to use a ref here
  InterruptController& interrupt_controller_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
};
