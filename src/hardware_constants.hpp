#pragma once

#include <cstdint>

namespace game_boy_memory {
  // Cartridge ROM & MBC (Memory Bank Controller)
  inline constexpr uint16_t kROMBank0Start{0x0000};
  inline constexpr uint16_t kROMBank0End{0x3FFF};
  inline constexpr uint16_t kROMBankSwitchStart{0x4000};
  inline constexpr uint16_t kROMBankSwitchEnd{0x7FFF};

  inline constexpr uint16_t kVRAMStart{0x8000};
  inline constexpr uint16_t kVRAMEnd{0x9FFF};

  inline constexpr uint16_t kCartridgeRAMStart{0xA000};
  inline constexpr uint16_t kCartridgeRAMEnd{0xBFFF};

  inline constexpr uint16_t kWRAMStart{0xC000};
  inline constexpr uint16_t kWRAMEnd{0xDFFF};

  inline constexpr uint16_t kEchoRAMStart{0xE000};
  inline constexpr uint16_t kEchoRAMEnd{0xFDFF};

  inline constexpr uint16_t kOAMStart{0xFE00};
  inline constexpr uint16_t kOAMEnd{0xFE9F};

  inline constexpr uint16_t kUnusableStart{0xFEA0};
  inline constexpr uint16_t kUnusableEnd{0xFEFF};

  inline constexpr uint16_t kIORegistersStart{0xFF00};
  inline constexpr uint16_t kIORegistersEnd{0xFF7F};

  inline constexpr uint16_t kHRAMStart{0xFF80};
  inline constexpr uint16_t kHRAMEnd{0xFFFE};

}  // namespace game_boy_memory

namespace io_registers {
  inline constexpr uint16_t kJoyp{0xFF00};     // joypad
  inline constexpr uint16_t kSB{0xFF01};       // Serial SB
  inline constexpr uint16_t kSC{0xFF02};       // Serial SC
  inline constexpr uint16_t kDIV{0xFF04};      // Divider
  inline constexpr uint16_t kTIMA{0xFF05};     // Timer Counter
  inline constexpr uint16_t kTMA{0xFF06};      // Timer Modulo
  inline constexpr uint16_t kTAC{0xFF07};      // Timer Control
  inline constexpr uint16_t kIF{0xFF0F};       // Interrupt Flag
  inline constexpr uint16_t kLCDC{0xFF40};     // LCD Control
  inline constexpr uint16_t kSTAT{0xFF41};     // LCD Status
  inline constexpr uint16_t kSCY{0xFF42};      // BG Scroll Y
  inline constexpr uint16_t kSCX{0xFF43};      // BG Scroll X
  inline constexpr uint16_t kLY{0xFF44};       // Current Scanline
  inline constexpr uint16_t kLYC{0xFF45};      // LY Compare
  inline constexpr uint16_t kDMA{0xFF46};      // OAM DMA Transfer
  inline constexpr uint16_t kBGP{0xFF47};      // BG Palette
  inline constexpr uint16_t kOBPZero{0xFF48};  // Sprite Palette 0
  inline constexpr uint16_t kOBPOne{0xFF49};   // Sprite Palette 1
  inline constexpr uint16_t kWY{0xFF4A};       // Window pos Y
  inline constexpr uint16_t kWX{0xFF4B};       // Window pos X
  inline constexpr uint16_t kIE{0xFFFF};       // Interrupt Enable

}  // namespace io_registers

namespace interrupts {
  inline constexpr uint8_t kVblankBit{0};
  inline constexpr uint16_t kVblankVector{0x0040};

  inline constexpr uint8_t kLcdStatBit{1};
  inline constexpr uint16_t kLcdStatVector{0x0048};

  inline constexpr uint8_t kTimerBit{2};
  inline constexpr uint16_t kTimerVector{0x0050};

  inline constexpr uint8_t kSerialBit{3};
  inline constexpr uint16_t kSerialVector{0x0058};

  inline constexpr uint8_t kJoypadBit{4};
  inline constexpr uint16_t kJoypadVector{0x0060};
}  // namespace interrupts
