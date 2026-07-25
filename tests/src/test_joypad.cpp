#include <doctest.h>

#include "joypad.hpp"
#include "joypad_button.hpp"
#include "joypad_constants.hpp"

TEST_CASE("Joypad Read") {
  constexpr auto kAorRightBit{0};
  constexpr auto kBorLeftBit{1};
  constexpr auto kSelectorUptBit{2};
  constexpr auto kStartorDownBit{3};

  SUBCASE("Read handles neither Select bits selected") {
    Joypad joypad{};

    // don't set anything to select

    // set arbitrary buttons, include one joypad input (kUp)
    auto const dummy_snapshot{static_cast<uint8_t>(1 << joypad::Button::kA | 1 << joypad::Button::kB | 1 << joypad::Button::kUp)};
    joypad.setSnapshot(dummy_snapshot);

    auto const constructed_joypad_reg{joypad.read()};

    // expect none of our inputs set, as neither buttons nor dpad select were set
    CHECK(((constructed_joypad_reg >> kAorRightBit) & 1) == 1);
    CHECK(((constructed_joypad_reg >> kBorLeftBit) & 1) == 1);
    CHECK(((constructed_joypad_reg >> kSelectorUptBit) & 1) == 1);
  }

  SUBCASE("Read returns Select Buttons") {
    Joypad joypad{};

    // set select button bit, active low so set it to 0
    auto const select_buttons{~(1 << joypad::kSelectButtonBit)};
    joypad.writeSelect(select_buttons);

    // set arbitrary buttons, include one joypad input (kUp)
    auto const dummy_snapshot{static_cast<uint8_t>(1 << joypad::Button::kA | 1 << joypad::Button::kB | 1 << joypad::Button::kUp)};
    joypad.setSnapshot(dummy_snapshot);

    auto const constructed_joypad_reg{joypad.read()};

    // expect left and right are set, but select shouldn't be, since we are in select button mode, once again joypad is active low, 0 = on
    CHECK(((constructed_joypad_reg >> kAorRightBit) & 1) == 0);
    CHECK(((constructed_joypad_reg >> kBorLeftBit) & 1) == 0);
    CHECK(((constructed_joypad_reg >> kSelectorUptBit) & 1) == 1);
  }

  SUBCASE("Read returns D Pad Buttons") {
    Joypad joypad{};

    // set select d pad bit, active low so set it to 0
    auto const select_d_pad{~(1 << joypad::kSelectDPadBit)};
    joypad.writeSelect(select_d_pad);

    // set arbitrary buttons, include one select button input (kA)
    auto const dummy_snapshot{static_cast<uint8_t>(1 << joypad::Button::kUp | 1 << joypad::Button::kDown | 1 << joypad::Button::kA)};
    joypad.setSnapshot(dummy_snapshot);

    auto const constructed_joypad_reg{joypad.read()};

    // expect select and start set, as d pad is set, expect left isn't set, active low, 0 = on
    CHECK(((constructed_joypad_reg >> kSelectorUptBit) & 1) == 0);
    CHECK(((constructed_joypad_reg >> kStartorDownBit) & 1) == 0);
    CHECK(((constructed_joypad_reg >> kAorRightBit) & 1) == 1);
  }

  SUBCASE("Read handles both Selects") {
    Joypad joypad{};

    // set select d pad and select button bit, active low so set it to 0
    auto const select_d_pad{~(1 << joypad::kSelectDPadBit | 1 << joypad::kSelectButtonBit)};
    joypad.writeSelect(select_d_pad);

    // set buttons, include both d pad and buttons
    auto const dummy_snapshot{static_cast<uint8_t>(1 << joypad::Button::kSelect | 1 << joypad::Button::kStart | 1 << joypad::Button::kLeft)};
    joypad.setSnapshot(dummy_snapshot);

    auto const constructed_joypad_reg{joypad.read()};

    // expect all three are set, since both button and d pad bit are on
    CHECK(((constructed_joypad_reg >> kSelectorUptBit) & 1) == 0);
    CHECK(((constructed_joypad_reg >> kStartorDownBit) & 1) == 0);
    CHECK(((constructed_joypad_reg >> kBorLeftBit) & 1) == 0);
  }
}

TEST_CASE("Joypad Write Select Filters Junk") {
  constexpr auto kSelectDPadBit{4};
  constexpr auto kSelectButtonBit{5};

  // a "clean" write selecting both lines: bits 4 and 5 low (active low = selected),
  // every other bit high. this is the reference behaviour.
  auto const clean_both_select{static_cast<uint8_t>(~(1 << joypad::kSelectDPadBit | 1 << joypad::kSelectButtonBit))};

  // a "junk" write selecting the same two lines: bits 4 and 5 still low, but every
  // other bit is the opposite of the clean write. only bits 4 and 5 carry meaning,
  // so kSelectDPadMask / kSelectButtonMask must strip everything else away.
  auto const junk_both_select{static_cast<uint8_t>(clean_both_select ^ 0xCF)};

  // same snapshot for both, so any read difference can only come from the select write
  auto const dummy_snapshot{static_cast<uint8_t>(1 << joypad::Button::kSelect | 1 << joypad::Button::kStart | 1 << joypad::Button::kLeft)};

  Joypad clean_joypad{};
  clean_joypad.writeSelect(clean_both_select);
  clean_joypad.setSnapshot(dummy_snapshot);

  Joypad junk_joypad{};
  junk_joypad.writeSelect(junk_both_select);
  junk_joypad.setSnapshot(dummy_snapshot);

  auto const clean_reg{clean_joypad.read()};
  auto const junk_reg{junk_joypad.read()};

  // anchor: both writes actually selected both lines (bits 4 and 5 read low)
  CHECK(((clean_reg >> kSelectDPadBit) & 1) == 0);
  CHECK(((clean_reg >> kSelectButtonBit) & 1) == 0);

  // the junk bits outside 4 and 5 were filtered, so the reads are identical
  CHECK(junk_reg == clean_reg);
}

TEST_CASE("Joypad Snapshot Overwrite") {
  constexpr auto kAorRightBit{0};
  constexpr auto kBorLeftBit{1};
  constexpr auto kSelectorUptBit{2};
  constexpr auto kStartorDownBit{3};

  // select both lines so read surfaces both the upper (buttons) and lower (d pad)
  // nibble of the snapshot, giving visibility into every stored bit
  auto const select_both{~(1 << joypad::kSelectDPadBit | 1 << joypad::kSelectButtonBit)};

  Joypad joypad{};
  joypad.writeSelect(select_both);

  // every button pressed
  joypad.setSnapshot(0xFF);

  auto const all_pressed_reg{joypad.read()};

  // active low: all four data bits read as pressed (0)
  CHECK(((all_pressed_reg >> kAorRightBit) & 1) == 0);
  CHECK(((all_pressed_reg >> kBorLeftBit) & 1) == 0);
  CHECK(((all_pressed_reg >> kSelectorUptBit) & 1) == 0);
  CHECK(((all_pressed_reg >> kStartorDownBit) & 1) == 0);

  // overwrite with an all-released snapshot. read folds the upper and lower nibble
  // together, so a fully released read is only possible if all eight prior bits were
  // cleared: proof that setSnapshot replaced everything rather than merging
  joypad.setSnapshot(0x00);

  auto const all_released_reg{joypad.read()};

  CHECK(((all_released_reg >> kAorRightBit) & 1) == 1);
  CHECK(((all_released_reg >> kBorLeftBit) & 1) == 1);
  CHECK(((all_released_reg >> kSelectorUptBit) & 1) == 1);
  CHECK(((all_released_reg >> kStartorDownBit) & 1) == 1);
}
