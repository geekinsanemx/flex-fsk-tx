# TTGO LoRa32-OLED FLEX Paging Message Transmitter Firmware

This directory contains the firmware files for TTGO LoRa32-OLED ESP32 development boards with SX1276 LoRa radio chipsets.

## 📋 Hardware Specifications

**TTGO LoRa32-OLED Development Board**
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio Chipset**: Semtech SX1276 LoRa/FSK transceiver
- **Frequency Range**: 433/868/915 MHz (region dependent)
- **TX Power**: 0-20 dBm (configurable)
- **Display**: 0.96" OLED (128x64 pixels, SSD1306)
- **Connectivity**: USB-C, WiFi 802.11 b/g/n, Bluetooth
- **Power**: USB-C or Li-Po battery connector
- **Serial Port**: Typically `/dev/ttyACM0` on Linux
- **Manufacturer**: LilyGO (TTGO)

## 🔧 Firmware Versions

### v1 Firmware: `ttgo_fsk_tx_AT.ino`
- **Basic AT command interface**
- Binary data transmission via `AT+SEND`
- Host-side FLEX encoding using tinyflex library
- Minimal memory footprint
- Compatible with original flex-fsk-tx host application

### v2 Firmware: `ttgo_fsk_tx_AT_v2.ino`
- **All v1 features plus:**
- On-device FLEX encoding via `AT+MSG` command
- Embedded tinyflex library integration
- Mail drop flag support (`AT+MAILDROP`)
- Enhanced error handling and status reporting
- Backward compatible with v1 host applications

### v3 Firmware: `ttgo_fsk_tx_AT_v3.ino`
- **All v2 features plus:**
- **WiFi connectivity** with station and AP modes
- **Web interface** for message transmission and configuration
- **REST API** with HTTP Basic Authentication (port 16180)
- **Theme support** - Default (Blue), Light, Dark themes
- **EEPROM configuration** - Persistent settings storage
- **Real-time validation** - Client and server-side parameter checking
- **Custom banner** - 16-character configurable display message
- **Battery monitoring** - Voltage and percentage display
- **Enhanced AT commands** for WiFi and system configuration

## 🚀 Quick Start

**🚀 New User?** See [QUICKSTART.md](../../QUICKSTART.md) for a complete beginner's guide from unboxing to first message!

### Prerequisites

1. **Hardware**: TTGO LoRa32-OLED development board
2. **Development Environment**: Arduino IDE with ESP32 support
3. **Required Libraries**:
   - **All versions**: RadioLib
   - **v3 firmware**: RadioBoards, U8g2, ArduinoJson (additional)

### Complete Installation Guide

**📱 Detailed Instructions**: See [FIRMWARE.md](../../FIRMWARE.md) for complete firmware installation procedures including:
- Library dependencies and installation steps
- Device-specific board configurations  
- Upload troubleshooting procedures
- tinyflex.h embedding requirements for v2/v3 firmware

#### Quick Library Reference
- **RadioLib** by Jan Gromeš (required for all versions)
- **U8g2** by oliver (for OLED display support)
- **ArduinoJson** by Benoit Blanchon (v3 firmware only)
- **RadioBoards** by radiolib-org (manual installation for v3 firmware)
3. **Flash Size**: 4MB (32Mb) typical for TTGO
4. **Partition Scheme**: Default 4MB with SPIFFS

### Firmware Selection Guide

Choose firmware based on your requirements:

#### Use v1 if:
- You want minimal memory usage
- You prefer host-side FLEX encoding
- You only need basic AT command functionality

#### Use v2 if:
- You want on-device FLEX encoding capabilities
- You need mail drop flag support
- You want enhanced error handling
- You prefer not to install tinyflex on the host

#### Use v3 if:
- You want WiFi connectivity and web interface
- You need remote configuration capabilities
- You want REST API access for automation
- You prefer browser-based message transmission
- You need theme customization
- You want persistent configuration storage

## 📱 v3 Firmware Web Interface

### Initial Setup (First Boot)
1. Flash v3 firmware to TTGO device
2. Power on - device creates WiFi AP: `TTGO_FLEX_XXXX`
3. Connect to AP using password: `12345678`
4. Open browser to: `http://192.168.4.1`
5. Configure your WiFi settings
6. Device will connect to your network

### Web Interface Pages
- **Main Interface** (`/`): Message transmission with real-time validation
- **Configuration** (`/configuration`): Device and network settings
- **Status** (`/status`): System information and factory reset

### REST API Usage
```bash
# Send message via API
curl -X POST http://DEVICE_IP:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929.6625,
    "power": 10,
    "message": "Hello from REST API"
  }'
```

## ⚙️ Configuration Examples

### v1/v2 Firmware AT Commands
```bash
# Connect via serial
screen /dev/ttyACM0 115200

# Basic configuration
AT+FREQ=929.6625
AT+POWER=10

# Send message (v2 firmware)
AT+MSG=1234567
# Device responds: +MSG: READY
Hello World
# Device responds: OK
```

### v3 Firmware WiFi Configuration
```bash
# Configure WiFi via AT commands
AT+WIFI=MyNetwork,MyPassword

# Configure device settings
AT+BANNER=My FLEX TX
AT+SAVE
AT+APIPORT=8080
AT+SAVE
```

## 🔧 Pin Configuration

### SX1276 Radio Interface
```cpp
#define LORA_NSS    18  // SPI Chip Select
#define LORA_NRESET 14  // Reset pin
#define LORA_DIO0   26  // Digital I/O 0 (IRQ)
#define LORA_SCK    5   // SPI Clock
#define LORA_MISO   19  // SPI MISO
#define LORA_MOSI   27  // SPI MOSI
```

### System Control
```cpp
#define LED_PIN     25  // Built-in LED
// OLED: I2C pins 4 (SDA) and 15 (SCL)
```

## 🚨 Troubleshooting

**🔧 Complete Troubleshooting**: See [TROUBLESHOOTING.md](../../TROUBLESHOOTING.md) for comprehensive TTGO-specific issue resolution covering firmware installation, hardware problems, and professional problem reporting.

### Quick TTGO Issues

**Compilation Errors**:
- Ensure all required libraries are installed
- Select correct board (TTGO LoRa32-OLED or ESP32 Dev Module)

**Upload Failures**:
- Put device in download mode: Hold BOOT → Press RESET → Release RESET → Release BOOT
- Try different upload speeds (115200, 921600)

**Device Not Responding**:
- Verify correct serial port (`/dev/ttyACM0`)
- Ensure antenna is connected (CRITICAL for radio operation)

### Pin Compatibility Notes

TTGO board revisions may have different pin configurations. If you encounter radio communication issues:

1. Check your specific board revision
2. Verify pin definitions in firmware match your hardware
3. Consult TTGO documentation or community resources
4. Consider using multimeter to verify pin connections

## 📊 Performance Specifications

- **Transmission Mode**: FSK (Frequency Shift Keying)
- **Bit Rate**: 1.6 kbps
- **Frequency Deviation**: 5 kHz
- **RX Bandwidth**: 10.4 kHz
- **Serial Baudrate**: 115200 bps
- **Max Message Length**: 240 characters (FLEX mode)
- **Max Binary Data**: 2048 bytes (binary mode)

## 🔗 Related Documentation

- **[FIRMWARE.md](../../FIRMWARE.md)**: Complete firmware setup guide
- **[AT_COMMANDS.md](../../AT_COMMANDS.md)**: Full AT command reference
- **[REST_API.md](../../REST_API.md)**: REST API documentation (v3 firmware)
- **[USER_GUIDE.md](../../USER_GUIDE.md)**: Web interface user guide (v3 firmware)
- **[CLAUDE.md](../../CLAUDE.md)**: Technical architecture notes

## ⚠️ Important Notes

1. **Antenna Required**: Never operate without proper antenna - can damage radio chipset
2. **Legal Compliance**: Verify frequency and power settings comply with local regulations
3. **Pin Verification**: Confirm pin definitions match your specific TTGO board revision
4. **Library Dependencies**: v3 firmware requires additional libraries (RadioBoards, U8g2, ArduinoJson)
5. **Firmware Selection**: v3 firmware provides WiFi and web interface capabilities

## 🤝 Support

For TTGO-specific issues:
1. Check existing documentation in this directory
2. Verify hardware compatibility and pin configurations
3. Test with basic examples before complex configurations
4. Consult TTGO/LilyGO community resources if needed

---

**Developed for the amateur radio and paging community**