#pragma once

#include <cstdint>

class Joypad {
 public:
  Joypad() = default;

  [[nodiscard]] uint8_t read() const;
  void writeSelect(uint8_t val);
  void setSnapshot(uint8_t snapshot) { joypad_buttons_ = snapshot; }

 private:
  // top 4 bits = select buttons, bottom 4 = d pad buttons
  uint8_t joypad_buttons_{};

  // both select lines are stored as one bit, despite being uint8_t, they'll only ever contain the value at their respective bit locations
  uint8_t select_buttons_{};
  uint8_t select_d_pad_{};
};
