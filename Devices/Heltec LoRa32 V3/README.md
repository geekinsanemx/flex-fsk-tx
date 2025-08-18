# Heltec WiFi LoRa 32 V3 FLEX Transmitter Firmware

This directory contains the firmware files for Heltec WiFi LoRa 32 V3 ESP32-S3 development boards with SX1262 LoRa radio chipsets.

## 📋 Hardware Specifications

**Heltec WiFi LoRa 32 V3 Development Board**
- **MCU**: ESP32-S3 (240MHz dual-core Xtensa LX7)
- **Radio Chipset**: Semtech SX1262 LoRa/FSK transceiver
- **Frequency Range**: 433/868/915 MHz (region dependent)
- **TX Power**: -9 to +22 dBm (configurable)
- **Display**: 0.96" OLED (128x64 pixels, SSD1306)
- **Connectivity**: USB-C, WiFi 802.11 b/g/n, Bluetooth 5.0
- **Power**: USB-C or Li-Po battery connector
- **Serial Port**: Typically `/dev/ttyUSB0` on Linux
- **Manufacturer**: Heltec Automation

## 🔧 Firmware Versions

### v1 Firmware: `heltec_fsk_tx_AT.ino`
**Encoding Type**: Local encoding (PC-side)
- **Basic AT command interface**
- Binary data transmission via `AT+SEND`
- Host-side FLEX encoding using tinyflex library
- Minimal memory footprint
- Compatible with original flex-fsk-tx host application
- Heltec-optimized OLED display integration

### v2 Firmware: `heltec_fsk_tx_AT_v2.ino`
**Encoding Type**: Remote encoding (device-side)
- **All v1 features plus:**
- On-device FLEX encoding via `AT+MSG` command
- Embedded tinyflex library integration
- Mail drop flag support (`AT+MAILDROP`) via AT commands
- Enhanced error handling and status reporting
- Backward compatible with v1 host applications
- Automatic Heltec power management (Vext control)

## 🚀 Quick Start

### Prerequisites

1. **Hardware**: Heltec WiFi LoRa 32 V3 development board
2. **Development Environment**: Arduino IDE with ESP32 support
3. **Required Libraries**:
   - **All versions**: RadioLib, Heltec ESP32 Library
   - **v2 firmware**: No additional libraries required (Heltec libraries handle OLED)

### Library Installation

#### Via Arduino IDE Library Manager
1. Open `Tools` → `Manage Libraries`
2. Install the following libraries:
   - **RadioLib** by Jan Gromeś (required for all versions)
   - **Heltec ESP32 Dev-Boards** by Heltec Automation (required for all versions)

### Board Configuration

1. **Select Board**: `Tools` → `Board` → `ESP32 Arduino` → `Heltec WiFi LoRa 32(V3)`
2. **Upload Speed**: 115200 or 921600
3. **Flash Size**: 8MB (64Mb) typical for Heltec V3
4. **Partition Scheme**: 8M Flash (3MB APP/1.5MB SPIFFS)

### Firmware Selection Guide

Choose firmware based on your requirements:

#### Use v1 if:
- You want minimal memory usage
- You prefer host-side FLEX encoding
- You only need basic AT command functionality
- You're using existing host applications

#### Use v2 if:
- You want on-device FLEX encoding capabilities
- You need mail drop flag support via AT commands
- You want enhanced error handling
- You prefer not to install tinyflex on the host
- You want simplified host application development

## ⚙️ Configuration Examples

### v1/v2 Firmware AT Commands
```bash
# Connect via serial
screen /dev/ttyUSB0 115200

# Basic configuration
AT+FREQ=929.6625
AT+POWER=10

# Send message (v1 - binary mode)
AT+SEND=12
Hello World

# Send message (v2 - remote encoding)
AT+MSG=1234567
# Device responds: +MSG: READY
Hello World
# Device responds: OK
```

### v2 Firmware Mail Drop Configuration
```bash
# Configure mail drop via AT commands
AT+MAILDROP=1     # Enable mail drop for next message
AT+MSG=1234567    # Send message with mail drop
Hello World
# Device responds: OK

AT+MAILDROP=0     # Disable mail drop (default)
```

## 🔧 Pin Configuration

### SX1262 Radio Interface (Heltec V3)
```cpp
// Heltec V3 uses Heltec ESP32 library pin definitions
// Typically configured automatically by Heltec library
#define LORA_NSS    8   // SPI Chip Select
#define LORA_NRESET 12  // Reset pin
#define LORA_BUSY   13  // Busy status
#define LORA_DIO1   14  // Digital I/O 1 (IRQ)
#define LORA_SCK    9   // SPI Clock
#define LORA_MISO   11  // SPI MISO
#define LORA_MOSI   10  // SPI MOSI
```

### System Control
```cpp
#define LED_PIN     35  // Built-in LED (active HIGH)
// OLED Display: I2C (auto-configured by Heltec library)
// Vext Power Control: Auto-managed by Heltec library
```

## 🚨 Troubleshooting

### Common Issues

**Compilation Errors**:
- Ensure Heltec ESP32 library is installed
- Select correct board: "Heltec WiFi LoRa 32(V3)"
- Update Arduino IDE and ESP32 core to latest versions

**Upload Failures**:
- Try different upload speeds (115200, 921600)
- Put device in download mode: Hold PRG → Press RST → Release RST → Release PRG
- Check USB cable and port permissions

**Device Not Responding**:
- Verify correct serial port (`/dev/ttyUSB0`)
- Check baud rate (115200)
- Press RST button on device
- Ensure antenna is connected (CRITICAL for radio operation)

**Display Issues**:
- Heltec library should handle OLED automatically
- If display remains blank, check Heltec library installation
- Try pressing RST button

**Radio Not Transmitting**:
- **Critical**: Check antenna connection (can damage radio if missing)
- Verify frequency is within SX1262 range (410-1000 MHz)
- Test with lower power settings first
- Ensure RadioLib compilation succeeded

### Heltec-Specific Notes

1. **Automatic Power Management**: Heltec library manages Vext power control automatically
2. **OLED Integration**: Display functionality is handled by Heltec ESP32 library
3. **Board Variants**: Ensure you have V3 (ESP32-S3 + SX1262), not V2 (ESP32 + SX1276)
4. **Pin Compatibility**: Heltec library provides correct pin definitions automatically

## 📊 Performance Specifications

- **Transmission Mode**: FSK (Frequency Shift Keying)
- **Bit Rate**: 1.6 kbps
- **Frequency Deviation**: 5 kHz
- **RX Bandwidth**: 10.4 kHz
- **Serial Baudrate**: 115200 bps
- **Max Message Length**: 240 characters (FLEX mode)
- **Max Binary Data**: 2048 bytes (binary mode)
- **Frequency Range**: 410-1000 MHz (SX1262 extended range)
- **Power Range**: -9 to +22 dBm (higher than TTGO)

## 🔗 Related Documentation

- **[FIRMWARE.md](../../FIRMWARE.md)**: Complete firmware setup guide for all devices
- **[AT_COMMANDS.md](../../AT_COMMANDS.md)**: Full AT command reference
- **[API.md](../../API.md)**: REST API documentation (v3 firmware only - TTGO)
- **[CLAUDE.md](../../CLAUDE.md)**: Technical architecture notes

## ⚠️ Important Notes

1. **Antenna Required**: Never operate without proper antenna - can damage SX1262 chipset
2. **Legal Compliance**: Verify frequency and power settings comply with local regulations
3. **Board Version**: Ensure you have Heltec V3 (ESP32-S3 + SX1262), not older versions
4. **Library Dependencies**: Always use Heltec ESP32 library for proper hardware support
5. **Power Range**: SX1262 supports wider frequency range and higher power than SX1276

## 🤝 Support

For Heltec-specific issues:
1. Check Heltec ESP32 library documentation
2. Verify hardware compatibility (V3 vs V2)
3. Test with basic Heltec library examples first
4. Consult Heltec community resources if needed

---

**Optimized for Heltec WiFi LoRa 32 V3 hardware**