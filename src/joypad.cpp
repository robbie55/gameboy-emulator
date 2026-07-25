#include "joypad.hpp"

#include "joypad_constants.hpp"

/*
 *
 * Joypad::read()
 *
 * handles taking our logically mapped bit values and returning in an active low format
 *
 * First, | against both select register members, setting or not setting either of the select bits
 * Then, for both buttons and d pad, create a mask of either 0 or FF, depending on select bit,
 * and & that against the respective nibble for dpad/buttons
 *
 * Finally, | both results of (mask & nibble), | against our return value, and return inverse, gameboy expects active low
 *
 */

uint8_t Joypad::read() const {
  uint8_t joypad_register{};

  // or against both buttons and d pad, should flip on whichever bit is set, or neither
  joypad_register |= (select_buttons_ | select_d_pad_);

  // for both buttons and dpad, check if select is on, if so mask is FF, if not mask is 00,
  // then & against each respective nibble, yielding the register
  auto const buttons_select{1 & (select_buttons_ >> joypad::kSelectButtonBit)};
  uint8_t const buttons_mask{static_cast<uint8_t>(-(buttons_select & 1))};
  auto const buttons_nibble{joypad_buttons_ >> 4};

  auto const d_pad_select{1 & (select_d_pad_ >> joypad::kSelectDPadBit)};
  uint8_t const d_pad_mask{static_cast<uint8_t>(-(d_pad_select & 1))};
  auto const d_pad_nibble{0x0F & joypad_buttons_};

  joypad_register |= ((d_pad_mask & d_pad_nibble) | (buttons_mask & buttons_nibble));

  return ~(joypad_register);
}

void Joypad::writeSelect(uint8_t const val) {
  // joypad reg is active low, we store active high for logical sense
  auto const active_high_val{~val};

  select_buttons_ = (active_high_val & joypad::kSelectButtonMask);
  select_d_pad_ = (active_high_val & joypad::kSelectDPadMask);
}
