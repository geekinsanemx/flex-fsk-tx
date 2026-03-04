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
   - **Local boards.h**: Board pin definitions included in firmware (no external library needed)
   - **Heltec ESP32 Dev-Boards** by Heltec Automation (Heltec devices only)

3. **Board Configuration**:
   - **Heltec V2**: "Heltec WiFi LoRa 32(V2)"
   - **TTGO**: "TTGO LoRa32-OLED" or "ESP32 Dev Module"

4. **Flash Firmware**:
   ```bash
   # Open firmware in Arduino IDE (unified firmware for both boards)
   arduino-ide Firmware/v3/flex_fsk_tx-v3.ino

   # Board selection: Edit line 147 in the .ino file:
   # For TTGO: #define TTGO_LORA32_V21
   # For Heltec V2: #define HELTEC_WIFI_LORA32_V2
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
- **Location**: `Firmware/[VERSION]/flex_fsk_tx-v[VERSION].ino`
- **Supported Hardware**:
  - Heltec WiFi LoRa 32 V2 (ESP32 + SX1276 chipset) **✅ FULLY SUPPORTED - 248 characters**
  - TTGO LoRa32-OLED (ESP32 + SX1276 chipset) **✅ FULLY SUPPORTED - 248 characters**
- **Board Selection**: Edit `#define TTGO_LORA32_V21` or `#define HELTEC_WIFI_LORA32_V2` at top of .ino file
- **Firmware Versions**:
  - v1: Basic AT commands, binary transmission only
  - v2: Enhanced with on-device FLEX encoding via `AT+MSG`
  - v3: Full WiFi capabilities with web interface, REST API, IMAP, MQTT, theme support, and improved displays
  - v4: All v3 features + GSM/2G cellular connectivity (SIM800L module) with WiFi/GSM failover
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
- `AT+SAVE` - Save configuration to NVS (v3 firmware)
- `AT+LOGS?N` - Query last N lines of persistent log, default 25 (v3.6+ firmware)
- `AT+RMLOG` - Delete persistent log file (v3.6+ firmware)
- `AT+NETWORK=<mode>` - Lock network transport mode: AUTO/WIFI/GSM/AP (v3.8 GSM firmware only)
- `AT+NETWORK?` - Query current network transport mode (v3.8 GSM firmware only)

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

**Persistent SPIFFS Log System** (v3.6.104+):
- **File**: `/serial.log` on SPIFFS, 250KB max with automatic rotation (keeps last 50KB)
- **Unified Logging**: Single `logMessage()` function handles Serial + file + syslog output
- **Timestamps**: Pre-NTP/RTC: `0000-00-00 HH:MM:SS` (uptime), Post-NTP/RTC: `YYYY-MM-DD HH:MM:SS`
- **AT Commands**: `AT+LOGS?N` (query last N lines, default 25), `AT+RMLOG` (delete log file)
- **Web Interface**: Status page displays logs with auto-scroll, configurable refresh (1-60s), line count (10-500), download button
- **REST Endpoint**: `GET /logs?lines=N` returns JSON array of log lines
- **Boot**: Logging starts immediately after SPIFFS init

**RTC Time Integration** (v3.6.105+):
- Log timestamps use `system_time_initialized` flag (works with both RTC and NTP)
- RTC-equipped devices get valid timestamps immediately at boot
- Boot sequence skips NTP blocking when RTC time is valid (faster boot)

**Network Transport Mode Control** (v3.8.52+, GSM firmware only):
- `AT+NETWORK=<mode>` locks transport to WiFi, GSM, AP, or AUTO
- Disables automatic failover when locked
- Retry intervals: WiFi 60s, GSM 300s, AP none
- Display shows asterisk indicator when locked (e.g., `WiFi*`)
- Resets to AUTO on reboot

**Display Management**:
- **AP Mode Content**: Shows "AP Mode Active", SSID, Password, and IP address
- **Font Optimization**: Smaller font for SSID to ensure full visibility
- **Layout Improvements**: Better spacing and positioning for readability
- **State Management**: Proper display updates when transitioning between modes

### Dependencies
- **tinyflex**: Git submodule at `include/tinyflex/` - FLEX protocol library
- **RadioLib**: Arduino library for radio control (all ESP32 firmware)
- **Device-Specific Libraries**:
  - **TTGO**: U8g2 (display), boards.h (local pin definitions)
  - **Heltec**: Heltec ESP32 Dev-Boards (display and power management)
- **v3 Firmware Additional**: ArduinoJson (REST API and web interface), RTClib (optional RTC support)
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

# Firmware files located at:
# Firmware/v3/flex_fsk_tx-v3.ino (unified for both boards)

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

# Test persistent log commands (v3.6.104+)
echo -e "AT+LOGS?25\r\n" | screen /dev/ttyACM0 115200  # Query last 25 log lines
echo -e "AT+LOGS?50\r\n" | screen /dev/ttyACM0 115200  # Query last 50 log lines
echo -e "AT+RMLOG\r\n" | screen /dev/ttyACM0 115200    # Delete log file

# Test network mode control (v3.8 GSM firmware only)
echo -e "AT+NETWORK?\r\n" | screen /dev/ttyACM0 115200         # Query current mode
echo -e "AT+NETWORK=WIFI\r\n" | screen /dev/ttyACM0 115200     # Lock to WiFi
echo -e "AT+NETWORK=AUTO\r\n" | screen /dev/ttyACM0 115200     # Return to auto
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
curl -X POST http://DEVICE_IP/api \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":10,"message":"API Test"}'

# Test message truncation via API (v3.1+)
curl -X POST http://DEVICE_IP/api \
  -u username:password \
  -H "Content-Type: application/json" \
  -d "{\"capcode\":1234567,\"frequency\":929.6625,\"power\":10,\"message\":\"$(printf '%*s' 250 '' | tr ' ' 'A')\"}"
# Response includes "truncated": true and indicates truncation occurred

# Test with Hz frequency format (auto-converts)
curl -X POST http://DEVICE_IP/api \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929662500,"power":10,"message":"Hz Format Test"}'

# Test log REST endpoint (v3.6.104+)
curl -s http://DEVICE_IP/logs?lines=50
# Returns JSON: {"logs":["line1","line2",...]}

# Download full log file
curl -s http://DEVICE_IP/download_logs -o serial.log

# Test API authentication
curl -X POST http://DEVICE_IP/api \
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
Firmware/                       # ESP32 firmware (unified for all boards)
├── v1/                         # Basic AT commands
│   ├── flex_fsk_tx_v1.ino     # Unified firmware
│   └── boards/                 # Local copy of board definitions
├── v2/                         # + FLEX encoding
│   ├── flex_fsk_tx_v2.ino     # Unified firmware
│   └── boards/                 # Local copy of board definitions
├── v3/                         # + WiFi/Web/API/IMAP/MQTT
│   ├── flex_fsk_tx-v3.ino     # Unified firmware
│   └── boards/                 # Local copy of board definitions
└── v4/                         # + GSM/2G support
    ├── flex_fsk_tx-v4.ino     # Unified firmware
    └── boards/                 # Local copy of board definitions
include/
└── boards/
    └── boards.h                # Master board pin definitions (TTGO/Heltec)
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

### REST API (Port 80, same as web interface)
- **Authentication**: HTTP Basic Auth with configurable credentials
- **Endpoint**: `POST /api` with JSON payload
- **Format**: `{"capcode": 1234567, "frequency": 929.6625, "power": 10, "message": "text"}`
- **Default Credentials**: `username:password` (configurable via AT commands)
- **Grafana Webhook**: `POST /api/v1/alerts` for Grafana alerts

## Device-Specific Notes

### TTGO LoRa32-OLED ✅
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Serial port**: `/dev/ttyACM0` (Linux), `COM3+` (Windows)
- **Radio**: SX1276 (433/868/915 MHz)
- **Power**: 0 to +20 dBm
- **Default frequency**: 915.0 MHz
- **Message length**: Up to 248 characters
- **Libraries**: RadioLib + U8g2 + ArduinoJson (v3), boards.h (local)
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
- **Firmware v3**: Full WiFi stack with web interface, REST API, theme support, NVS + SPIFFS configuration, and enhanced display management (current)
  - **v3.1 Features**: EMR (Emergency Message Resynchronization) support, message truncation instead of rejection
  - **v3.6.104+**: Persistent SPIFFS log system (`AT+LOGS?N`, `AT+RMLOG`), `/logs` REST endpoint, status page live log display
  - **v3.6.105+**: RTC time integration for immediate boot timestamps
  - **v3.8.52+**: Network transport mode control (`AT+NETWORK` command, GSM firmware only)
  - **Recent Updates**: Improved AP mode display, consistent SSID generation, better OLED management
  - **Current Status**: Active development with logging, diagnostics, and user experience improvements
- **tinyflex**: Single-header FLEX protocol library (stable submodule)

## Firmware Evolution

- **v1**: Basic AT commands with binary transmission
- **v2**: Enhanced with on-device FLEX encoding
- **v3**: Full WiFi capabilities with web interface and REST API (recommended for WiFi-only)
- **v4**: v3 + GSM/2G cellular connectivity with automatic WiFi/GSM failover (recommended for cellular backup)

## Current Architecture

The v3/v4 firmware represents the current state-of-the-art with enhanced capabilities:

### Primary Interfaces
1. **Web Interface** (Port 80): Primary user interface for message transmission and configuration
2. **REST API** (Port 80, `/api` endpoint): Automated access with HTTP Basic Auth
3. **AT Commands**: Backward compatibility and advanced configuration

### Development Focus
- **Standalone Operation**: Device operates independently with web interface
- **Network Connectivity**: WiFi-first design (v3) or WiFi/GSM dual-transport (v4)
- **Enhanced Configuration**: NVS + SPIFFS persistent settings (NVS for core config, SPIFFS for application data)
- **Theme Support**: Multiple UI themes with real-time switching
- **Arduino IDE Workflow**: Firmware development using standard Arduino IDE ecosystem

### v4 GSM Features
- **SIM800L Module Support**: 2G/GPRS cellular connectivity
- **Automatic Failover**: WiFi fails → GSM, GSM fails → AP mode
- **Dual Transport**: MQTT/IMAP/ChatGPT work over WiFi or GSM
- **Network Management**: Automatic WiFi reconnection attempts every 5 minutes when GSM active

### Arduino IDE Troubleshooting

**🔧 Complete Development Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for detailed Arduino IDE setup, library installation, and firmware development issue resolution.

**Quick Arduino IDE Issues**:
```bash
# Board detection issues
ls /dev/tty*  # Check available ports

# Verify all libraries installed via Library Manager:
# Tools → Manage Libraries → Search for:
# - RadioLib, U8g2, ArduinoJson (v3)
# - ReadyMail, PubSubClient, RTClib (v3, RTClib optional)
# - TinyGSM, SSLClient (v4)

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
- **Device Consistency**: Unified firmware with `include/boards/boards.h` ensures consistent UX between TTGO and Heltec devices
- **Board Selection**: Change board via `#define TTGO_LORA32_V21` or `#define HELTEC_WIFI_LORA32_V2` at top of .ino file
- **Pin Definitions**: Master definitions in `include/boards/boards.h`, local copies in `Firmware/v[X]/boards/boards.h`