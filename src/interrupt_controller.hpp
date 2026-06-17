#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

/*
 *
 * Interrupt Controller
 * Serves as a seam of the MMU, since other components will need to touch this section of the MMU,
 * we abstract it to the high level so lower level modules only ever depend on modules above them
 *
 */

class InterruptController {
 public:
  InterruptController() : interrupt_flag_{0xE1} {}

  void setEnable(std::byte interrupt_enable) { interrupt_enable_ = interrupt_enable; }
  void setFlag(std::byte interrupt_flag) { interrupt_flag_ = interrupt_flag; }

  // hardware won't ever use the top 3 bits, ensuring they're set properly -> v2
  [[nodiscard]] std::byte getEnable() const { return interrupt_enable_; }
  [[nodiscard]] std::byte getFlag() const { return interrupt_flag_; }

  [[nodiscard]] std::optional<int> queryPendingInterrupt() const;

  void requestInterrupt(uint8_t bit);

  void clearFlagBit(uint8_t bit);

 private:
  std::byte interrupt_enable_{};
  std::byte interrupt_flag_{};
};
