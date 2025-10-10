# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**flex-fsk-tx** is a FLEX pager message transmission system for ESP32 LoRa32 devices. The project consists of:

1. **Host Application** (`host/flex-fsk-tx.cpp`) - C++ application that encodes FLEX messages and communicates with ESP32 via AT commands
2. **ESP32 Firmware** - Arduino firmware for supported LoRa32 devices that handles FSK transmission
3. **tinyflex Library** - Submodule dependency providing FLEX protocol implementation

## Supported Hardware

- **TTGO LoRa32-OLED** (ESP32 + SX1276) - ✅ Fully supported, 248-character messages
- **Heltec WiFi LoRa 32 V2** (ESP32 + SX1276) - ✅ Fully supported, 248-character messages

**Note**: Heltec WiFi LoRa 32 V3 (SX1262 chipset) is not supported due to critical transmission issues with chunking protocols that limit message length to ~130 characters. Use Heltec V2 or TTGO devices instead.

## Build System

### Host Application (C++)

```bash
# Build the host application
make

# Build with debug symbols  
make debug

# Install to system (requires sudo)
sudo make install

# Clean build artifacts
make clean

# Check dependencies (tinyflex submodule)
make check-deps

# Show available targets
make help
```

**Prerequisites**:
- C++ compiler (g++)
- tinyflex submodule: `git submodule update --init --recursive`

### ESP32 Firmware (Arduino IDE)

**v3 Firmware Development Workflow**:

1. **Setup Arduino IDE**:
   - Install ESP32 board support via Board Manager
   - Add ESP32 boards URL: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`

2. **Install Required Libraries** (via Library Manager):
   - **RadioLib** by Jan Gromeš (all devices, all versions)
   - **U8g2** by oliver (TTGO devices only)
   - **ArduinoJson** by Benoit Blanchon (v3 firmware only)
   - **RadioBoards**: Manual install from `https://github.com/radiolib-org/RadioBoards.git` (TTGO v3 only)
   - **Heltec ESP32 Dev-Boards** by Heltec Automation (Heltec devices only)

3. **Board Configuration**:
   - **Heltec V2**: "Heltec WiFi LoRa 32(V2)"
   - **TTGO**: "TTGO LoRa32-OLED" or "ESP32 Dev Module"

4. **Flash Firmware**:
   ```bash
   # Open firmware in Arduino IDE
   arduino-ide Devices/TTGO_LoRa32/firmware/v3/ttgo_fsk_tx_AT_v3.ino

   # Or for Heltec V2
   arduino-ide Devices/Heltec_WiFi_LoRa32_V2/firmware/v3/heltec_fsk_tx_AT_v3.ino
   ```

## Architecture

### Host Application (host/flex-fsk-tx.cpp)
- **Serial Communication**: AT command protocol for ESP32 communication
- **FLEX Encoding**: Uses tinyflex library for local message encoding (default mode)
- **Dual Encoding Modes**:
  - Local encoding: Host encodes FLEX messages, sends binary via `AT+SEND`
  - Remote encoding (`-r` flag): Device encodes using `AT+MSG` command (v2 firmware)
- **Input Processing**: Command line args or stdin with loop mode support

### ESP32 Firmware
- **Location**: `Devices/[DEVICE_NAME]/firmware/[VERSION]/[DEVICE]_fsk_tx_AT_[VERSION].ino`
- **Supported Hardware**:
  - Heltec WiFi LoRa 32 V2 (ESP32 + SX1276 chipset) **✅ FULLY SUPPORTED - 248 characters**
  - TTGO LoRa32-OLED (ESP32 + SX1276 chipset) **✅ FULLY SUPPORTED - 248 characters**
- **Firmware Versions**:
  - v1: Basic AT commands, binary transmission only
  - v2: Enhanced with on-device FLEX encoding via `AT+MSG`
  - v3: Full WiFi capabilities with web interface, REST API, IMAP, MQTT, theme support, and improved displays
    - Enhanced AP mode display (shows essential info without battery clutter)
    - Consistent 4-character SSID generation for both devices
    - Periodic display refresh in AP mode
    - Better OLED positioning and font management

### AT Command Protocol
Standardized command set for ESP32 communication:
- `AT+FREQ=<MHz>` - Set frequency
- `AT+POWER=<dBm>` - Set transmit power  
- `AT+SEND=<bytes>` - Binary data transmission
- `AT+MSG=<capcode>` - FLEX message (v2+ firmware)
- `AT+MAILDROP=<0|1>` - Mail drop flag (v2+ firmware)
- `AT+WIFI=<ssid>,<password>` - Configure and connect to WiFi (v3 firmware)
- `AT+BANNER=<text>` - Set custom banner message (v3 firmware)
- `AT+APIPORT=<port>` - Set REST API port (v3 firmware)
- `AT+BATTERY?` - Query battery status (v3 firmware)
- `AT+SAVE` - Save configuration to EEPROM (v3 firmware)

### Recent v3 Firmware Improvements (Current Branch)

**v3.1 Major Features**:
- **EMR (Emergency Message Resynchronization)**: Improves pager synchronization by sending sync bursts
  - Triggers on first message or after 10-minute timeout since last EMR
  - Uses sync pattern {0xA5, 0x5A, 0xA5, 0x5A} at current radio settings
  - Unified transmission: all FLEX messages → Queue → EMR → Radio
- **Message Truncation**: Auto-truncates messages instead of rejecting them
  - Truncates messages longer than 248 characters at 245 chars + "..." (248 total)
  - Web/API responses indicate when truncation occurred
  - Character counter shows truncation warnings at 245+ characters

**Enhanced AP Mode Display**:
- **Heltec Devices**: Fixed AP mode display to show essential connection information clearly
- **Consistent SSID Format**: Both TTGO and Heltec now use 4-character hex suffixes
- **Battery Information**: Moved battery display out of AP mode to reduce clutter
- **Display Positioning**: Improved OLED text positioning and font sizing for better visibility
- **Periodic Refresh**: Added display refresh every 1 second in AP mode to maintain visibility

**SSID Generation Standardization**:
- **TTGO**: `TTGO_FLEX_XXXX` (4 hex characters, e.g., TTGO_FLEX_A1B2)
- **Heltec**: `HELTEC_FLEX_XXXX` (4 hex characters, e.g., HELTEC_FLEX_C3D4)
- **Algorithm**: Uses last 16 bits of MAC address for consistent 4-character format

**Display Management**:
- **AP Mode Content**: Shows "AP Mode Active", SSID, Password, and IP address
- **Font Optimization**: Smaller font for SSID to ensure full visibility
- **Layout Improvements**: Better spacing and positioning for readability
- **State Management**: Proper display updates when transitioning between modes

### Dependencies
- **tinyflex**: Git submodule at `include/tinyflex/` - FLEX protocol library
- **RadioLib**: Arduino library for radio control (all ESP32 firmware)
- **Device-Specific Libraries**:
  - **TTGO**: RadioBoards (manual), U8g2 (display)
  - **Heltec**: Heltec ESP32 Dev-Boards (display and power management)
- **v3 Firmware Additional**: ArduinoJson (REST API and web interface)
- **Manual Embedding**: tinyflex.h must be copied to firmware directory for v2/v3

## Common Development Tasks

### Testing Communication

**Host Application Testing**:
```bash
# Test with Heltec V2 device (local encoding)
echo "1234567:Test Message" | ./host/bin/flex-fsk-tx -d /dev/ttyUSB0 -

# Test with TTGO device (remote encoding)
echo "1234567:Test Message" | ./host/bin/flex-fsk-tx -d /dev/ttyACM0 -r -

# Test AT commands directly via serial (Heltec V2)
screen /dev/ttyUSB0 115200

# Test AT commands directly via serial (TTGO)
screen /dev/ttyACM0 115200
```

**v3 Firmware Direct Testing**:
```bash
# Test AT commands with v3 firmware (adjust port for device type)
echo -e "AT\r\nAT+FREQ=929.6625\r\nAT+POWER=10\r\nAT+MSG=1234567,Hello v3\r\n" | screen /dev/ttyACM0 115200  # TTGO
echo -e "AT\r\nAT+FREQ=929.6625\r\nAT+POWER=10\r\nAT+MSG=1234567,Hello v3\r\n" | screen /dev/ttyUSB0 115200  # Heltec

# Test message truncation (v3.1+)
echo -e "AT+MSG=1234567,$(printf '%*s' 250 '' | tr ' ' 'A')\r\n" | screen /dev/ttyACM0 115200
# Should truncate to 245 chars + "..." = 248 total

# Test EMR synchronization (automatic on first message or 10-minute timeout)
echo -e "AT+MSG=1234567,Test EMR sync\r\n" | screen /dev/ttyACM0 115200

# Test WiFi configuration and status
echo -e "AT+WIFI=MyNetwork,Password123\r\n" | screen /dev/ttyACM0 115200

# Check device status and battery (v3 only)
echo -e "AT+STATUS\r\nAT+WIFI?\r\nAT+BATTERY?\r\n" | screen /dev/ttyACM0 115200

# Test AP mode display improvements
echo -e "AT+WIFIENABLE=0\r\nAT+WIFIENABLE=1\r\n" | screen /dev/ttyACM0 115200
# Check OLED display for: "AP Mode Active", SSID, Password, IP
```

### Testing Web Interface

**Web Interface Testing**:
```bash
# Test web interface accessibility
curl -s http://DEVICE_IP/ | grep -o "flex-fsk-tx"

# Test configuration page
curl -s http://DEVICE_IP/configuration | grep -o "WiFi Configuration"

# Test status page
curl -s http://DEVICE_IP/status | grep -o "System Information"
```

**REST API Testing**:
```bash
# Test API with default credentials
curl -X POST http://DEVICE_IP:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":10,"message":"API Test"}'

# Test message truncation via API (v3.1+)
curl -X POST http://DEVICE_IP:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d "{\"capcode\":1234567,\"frequency\":929.6625,\"power\":10,\"message\":\"$(printf '%*s' 250 '' | tr ' ' 'A')\"}"
# Response includes "truncated": true and indicates truncation occurred

# Test with Hz frequency format (auto-converts)
curl -X POST http://DEVICE_IP:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929662500,"power":10,"message":"Hz Format Test"}'

# Test API authentication
curl -X POST http://DEVICE_IP:16180/ \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"message":"Auth Test"}' \
  -w "HTTP Status: %{http_code}\n"
```

**WiFi Configuration Testing**:
```bash
# Configure WiFi via AT commands
echo -e "AT+WIFI=MyNetwork,Password123\r\n" | screen /dev/ttyACM0 115200

# Test AP mode (if WiFi fails)
# Device creates WiFi network with password "12345678":
# - TTGO devices: "TTGO_FLEX_XXXX" (4 hex characters)
# - Heltec devices: "HELTEC_FLEX_XXXX" (4 hex characters)
# Access configuration at http://192.168.4.1

# Test device configuration
echo -e "AT+BANNER=My FLEX TX\r\nAT+SAVE\r\n" | screen /dev/ttyACM0 115200
```

### Flashing ESP32 Firmware

**Complete Instructions**: See [FIRMWARE.md](FIRMWARE.md) for detailed firmware installation procedures including:
- Library dependencies and installation steps
- Device-specific board configurations  
- tinyflex.h embedding requirements for v2/v3 firmware
- Upload troubleshooting and verification procedures
- Device-specific flashing procedures for both TTGO and Heltec hardware

### Device Detection
- **Heltec WiFi LoRa 32 V2**: Usually `/dev/ttyUSB0` (Linux), `COM4+` (Windows)
- **TTGO LoRa32-OLED**: Usually `/dev/ttyACM0` (Linux), `COM3+` (Windows)
- Check with: `ls /dev/tty*` or `dmesg | tail` (Linux)

## Code Patterns

### Error Handling
- AT command retries with exponential backoff
- Serial buffer flushing on communication errors
- Timeout management for different operation types

### Encoding Modes
```cpp
// Local encoding (default)
if (!remote_encoding) {
    // Use tinyflex library to encode message
    // Send binary data via AT+SEND
}

// Remote encoding (-r flag)
if (remote_encoding) {
    // Send capcode and text via AT+MSG
    // Device performs FLEX encoding
}
```

### Input Processing
- Single message: `flex-fsk-tx 1234567 "message"`
- Stdin mode: `echo "1234567:message" | flex-fsk-tx -`
- Loop mode: Multiple messages from stdin with `-l` flag

## File Structure

```
host/                           # Host application directory
├── flex-fsk-tx.cpp             # Main C++ source
├── Makefile                    # Build configuration
├── bin/                        # Build output
├── obj/                        # Object files
└── README.md                   # Host application documentation
include/tinyflex/               # FLEX protocol library (git submodule)
Devices/                        # ESP32 firmware by device type
├── TTGO_LoRa32/               # TTGO-specific firmware
│   └── firmware/
│       ├── v1/                 # Basic AT commands
│       ├── v2/                 # + FLEX encoding
│       └── v3/                 # + WiFi/Web/API/IMAP/MQTT
└── Heltec_WiFi_LoRa32_V2/     # Heltec V2-specific firmware
    └── firmware/
        ├── v1/                 # Basic AT commands
        ├── v2/                 # + FLEX encoding
        └── v3/                 # + WiFi/Web/API/IMAP/MQTT
docs/                           # Documentation
├── AT_COMMANDS.md              # AT command reference
├── FIRMWARE.md                 # Firmware installation guide
├── QUICKSTART.md               # Beginner's guide
├── REST_API.md                 # REST API documentation
├── TROUBLESHOOTING.md          # Troubleshooting guide
└── USER_GUIDE.md               # Web interface guide
README.md                       # Project overview
CLAUDE.md                       # Claude Code guidance (this file)
```

## Web Architecture (v3 Firmware)

### Web Interfaces (Port 80)
- **Main Interface** (`/`): Message transmission page with real-time validation
- **Configuration** (`/configuration`): Device settings and WiFi configuration  
- **Status** (`/status`): System information and factory reset

### REST API (Port 16180)
- **Authentication**: HTTP Basic Auth with configurable credentials
- **Endpoint**: `POST /` with JSON payload
- **Format**: `{"capcode": 1234567, "frequency": 929.6625, "power": 10, "message": "text"}`
- **Default Credentials**: `username:password` (configurable via AT commands)

## Device-Specific Notes

### TTGO LoRa32-OLED ✅
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Serial port**: `/dev/ttyACM0` (Linux), `COM3+` (Windows)
- **Radio**: SX1276 (433/868/915 MHz)
- **Power**: 0 to +20 dBm
- **Default frequency**: 915.0 MHz
- **Message length**: Up to 248 characters
- **Libraries**: RadioLib + RadioBoards (manual) + U8g2 + ArduinoJson (v3)
- **Display**: 128x64 OLED (U8g2 library)
- **AP SSID Format**: `TTGO_FLEX_XXXX` (4 hex chars)
- **Status**: ✅ Fully supported

### Heltec WiFi LoRa 32 V2 ✅
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Serial port**: `/dev/ttyUSB0` (Linux), `COM4+` (Windows)
- **Radio**: SX1276 (433/868/915 MHz)
- **Power**: 0 to +20 dBm
- **Default frequency**: 929.6625 MHz
- **Message length**: Up to 248 characters
- **Libraries**: RadioLib + Heltec ESP32 Dev-Boards + ArduinoJson (v3)
- **Display**: 128x64 OLED (Heltec library)
- **AP SSID Format**: `HELTEC_FLEX_XXXX` (4 hex chars)
- **Pin Differences**: VEXT power control (GPIO 21), different SPI/I2C pins
- **Status**: ✅ Fully supported

**Note**: Both devices use the SX1276 chipset and support full 248-character FLEX messages. Heltec V3 (SX1262) is not supported due to transmission issues.

## Debugging

### Enable Debug Output
Look for debug defines in firmware:
```cpp
// Uncomment for debug messages
// #define DEBUG_SERIAL 1
```

### Common Issues

**🔧 Complete Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive issue resolution covering all firmware versions, hardware problems, and development environment setup.

**Quick Development Issues**:
- **Device not responding**: Check serial port, try different cable
- **AT+MSG not recognized**: Ensure v2+ firmware is flashed
- **Permission denied**: Add user to dialout group
- **Compilation errors**: Verify all Arduino libraries installed

## Version Information

- **Host Application**: C++11 compatible, stable
- **Firmware v1**: Basic AT commands, binary transmission (stable)
- **Firmware v2**: Enhanced with remote FLEX encoding support (stable)
- **Firmware v3**: Full WiFi stack with web interface, REST API, theme support, EEPROM configuration, and enhanced display management (current)
  - **v3.1 Features**: EMR (Emergency Message Resynchronization) support, message truncation instead of rejection
  - **Recent Updates**: Improved AP mode display, consistent SSID generation, better OLED management
  - **Current Status**: Active development with display and user experience improvements
- **tinyflex**: Single-header FLEX protocol library (stable submodule)

## Firmware Evolution

- **v1**: Basic AT commands with binary transmission
- **v2**: Enhanced with on-device FLEX encoding
- **v3**: Full WiFi capabilities with web interface and REST API (current/recommended)

## Current Architecture

The v3 firmware represents the current state-of-the-art with enhanced capabilities:

### Primary Interfaces
1. **Web Interface** (Port 80): Primary user interface for message transmission and configuration
2. **REST API** (Port 16180): Automated access with HTTP Basic Auth
3. **AT Commands**: Backward compatibility and advanced configuration

### Development Focus
- **Standalone Operation**: Device operates independently with web interface
- **WiFi-First Design**: Network connectivity is primary interface
- **Enhanced Configuration**: EEPROM-based persistent settings
- **Theme Support**: Multiple UI themes with real-time switching
- **Arduino IDE Workflow**: Firmware development using standard Arduino IDE ecosystem

### Arduino IDE Troubleshooting

**🔧 Complete Development Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for detailed Arduino IDE setup, library installation, and firmware development issue resolution.

**Quick Arduino IDE Issues**:
```bash
# Library installation issues
cd ~/Arduino/libraries/
git clone https://github.com/radiolib-org/RadioBoards.git

# Board detection issues
ls /dev/tty*  # Check available ports
# Verify all libraries installed:
# Tools → Manage Libraries → Search for: RadioLib, U8g2, ArduinoJson

# Upload issues
# Try different upload speeds in Tools → Upload Speed
# Try different USB cables
# Hold BOOT button during upload for some boards
```

**Firmware Development Workflow**:
1. **Setup Environment**: Install Arduino IDE and device-specific libraries
2. **Embed Dependencies**: Copy `tinyflex.h` to firmware directory for v2/v3 
3. **Modify Firmware**: Edit `.ino` file in Arduino IDE
4. **Verify Compilation**: Sketch → Verify/Compile (check for library errors)
5. **Upload Firmware**: Sketch → Upload (use device-specific upload procedures)
6. **Basic Testing**: Test via Serial Monitor (115200 baud) with AT commands
7. **Display Verification**: Check OLED shows correct information and formatting
8. **WiFi Testing**: Test WiFi connection and AP mode (v3 firmware)
9. **Interface Testing**: Validate web interface and REST API functionality (v3 firmware)
10. **Documentation Update**: Update relevant .md files for any changes

**Display Development Notes**:
- **AP Mode Display**: Should show "AP Mode Active", SSID, Password, IP (without battery)
- **Font Management**: Use smaller fonts for SSID if needed for visibility
- **Periodic Updates**: Ensure display refreshes appropriately in different modes
- **Device Consistency**: Maintain similar UX between TTGO and Heltec devices

## 🏷️ Version Management System (2025)

### Current Version: 3.2.0
**Versioning**: [Semantic Versioning](https://semver.org/) (MAJOR.MINOR.PATCH)

### 🤖 Automated Version Management for Claude Code

**CRITICAL**: Before making ANY changes, always check current version and follow version management procedures.

#### Step 1: Check Current Version and Status
```bash
# Get comprehensive version information
./scripts/version-info.sh

# Quick version check
cat VERSION
```

#### Step 2: Classify Your Changes
- **PATCH** (3.2.Z+1): Bug fixes, documentation, UI improvements
- **MINOR** (3.Y+1.0): New features, hardware support, backward-compatible additions  
- **MAJOR** (X+1.0.0): Breaking changes, API incompatibility

#### Step 3: Use Automated Version Bump
```bash
# For bug fixes, documentation updates
./scripts/bump-version.sh patch "Fix antenna detection bug"

# For new features, hardware support
./scripts/bump-version.sh minor "Add message scheduling feature"

# For breaking changes  
./scripts/bump-version.sh major "Redesign AT command protocol"
```

### Version Increment Decision Matrix

| Change Type | Examples | Version | Command |
|-------------|----------|---------|---------|
| **Breaking Change** | AT protocol changes, API removal | MAJOR | `./scripts/bump-version.sh major "description"` |
| **New Feature** | Queue system, WiFi support, new hardware | MINOR | `./scripts/bump-version.sh minor "description"` |
| **Bug Fix/Docs** | Stability fixes, documentation, UI polish | PATCH | `./scripts/bump-version.sh patch "description"` |

### 🔄 Automated Process
The version bump script automatically:
1. ✅ Validates current version and git status
2. ✅ Calculates new version based on increment type
3. ✅ Creates timestamped backups of all modified files
4. ✅ Updates VERSION, README.md, CHANGELOG.md, Makefile, firmware files
5. ✅ Creates git commit with standardized message
6. ✅ Creates git tag with release notes
7. ✅ Displays summary and next steps

## Current Development Status (2025)

### Recent Improvements Completed
✅ **EMR (Emergency Message Resynchronization)**: Improves pager sync with automatic sync bursts (v3.1.0)
✅ **Message Truncation**: Auto-truncates long messages instead of rejection (v3.1.0)  
✅ **Message Queue System**: Up to 10 concurrent requests (v3.2.0)  
✅ **Enhanced AP Mode Display**: Fixed Heltec display issues, improved visibility (v3.1.0)
✅ **Consistent SSID Generation**: Both devices use 4-character hex format (v3.1.0) 
✅ **Display Optimization**: Better font sizing and positioning (v3.1.0)
✅ **Version Management System**: Automated SemVer with scripts (v3.2.0)
✅ **Documentation Updates**: All .md files updated to reflect current functionality

### Active Development Focus
🔄 **Version Consistency**: All changes must follow SemVer automated procedures
🔄 **User Experience**: Continued refinement of OLED display management  
🔄 **Code Quality**: Maintaining consistency between TTGO and Heltec implementations  
🔄 **Documentation**: Keeping all guides current with firmware improvements  

### 🚨 CRITICAL Development Guidelines for Claude Code
1. **ALWAYS start with version check**: Run `./scripts/version-info.sh` first
2. **Use automated version management**: Never manually update version numbers
3. **Create timestamped backups**: Script does this automatically
4. **Follow SemVer strictly**: Use decision matrix to classify changes
5. **Update CHANGELOG.md**: Script creates template, add detailed changes
6. **Test thoroughly**: Ensure version references are consistent
7. **Preserve existing functionality**: Don't break working features
8. **Consider user impact**: Especially for MAJOR version changes

### Version-Aware Development Workflow
1. **Analysis**: `./scripts/version-info.sh` - understand current state
2. **Classification**: Determine if changes are MAJOR/MINOR/PATCH
3. **Implementation**: Make code changes following established patterns
4. **Version Bump**: `./scripts/bump-version.sh [type] "description"`
5. **Validation**: Verify all files updated correctly
6. **Documentation**: Complete CHANGELOG.md entry with details

### Important File Patterns
- **Firmware Files**: `Devices/[DEVICE]/[DEVICE]_fsk_tx_AT_v3.ino`
- **Backup Pattern**: `filename.ext.bkp-YYMMDDHHMMSS`
- **Documentation**: Keep all .md files synchronized with firmware capabilities
- **Dependencies**: Remember tinyflex.h embedding for v2/v3 firmware development