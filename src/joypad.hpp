#pragma once

#include "interrupt_controller.hpp"

class Joypad {
 public:
  explicit Joypad(InterruptController& interrupt_controller) : interrupt_controller_{interrupt_controller} {};

  [[nodiscard]] uint8_t read() const;
  void setSnapshot(uint8_t snapshot) { joypad_buttons_ = snapshot; }

 private:
  // top 4 bits = select buttons, bottom 4 = d pad buttons
  uint8_t joypad_buttons_{};

  // both select lines are stored as one bit, despite being uint8_t, they'll only ever contain the value at their respective bit locations
  uint8_t select_buttons_{};
  uint8_t select_d_pad_{};

  // non owning relationship, OK to use a ref here
  InterruptController& interrupt_controller_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
};
