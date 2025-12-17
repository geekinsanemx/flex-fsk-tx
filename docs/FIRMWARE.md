# FLEX Paging Message Transmitter - Firmware Installation Guide

Complete guide for flashing firmware to ESP32 LoRa32 devices for FLEX paging transmission.

## 🎯 Quick Reference

| Device | Firmware | Directory | Key Features | Libraries Required | Status |
|--------|----------|-----------|--------------|--------------------|--------|
| **TTGO LoRa32-OLED** | v3.8 GSM (WiFi + GSM) | `Firmware/flex-fsk-tx-v3.8_GSM/` | WiFi + GSM/LTE failover, REST API, IMAP, MQTT, ChatGPT | RadioLib, U8g2, ArduinoJson, ReadyMail, PubSubClient, RTClib, TinyGSM, SSLClient + `tinyflex/` | ✅ **FULLY SUPPORTED** |
| **TTGO LoRa32-OLED** | v3.6 WiFi | `Firmware/flex-fsk-tx-v3.6_WiFi/` | WiFi + Web Interface + REST API | RadioLib, U8g2, ArduinoJson, ReadyMail, PubSubClient, RTClib + `tinyflex/` | ✅ **FULLY SUPPORTED** |
| **TTGO LoRa32-OLED** | v2 | `Firmware/flex-fsk-tx-v2/` | On-device FLEX encoding via AT+MSG | RadioLib, U8g2 + `tinyflex/` | ✅ **FULLY SUPPORTED** |
| **TTGO LoRa32-OLED** | v1 | `Firmware/flex-fsk-tx-v1/` | Basic AT commands, binary transmission | RadioLib, U8g2 | ✅ **FULLY SUPPORTED** |
| **Heltec WiFi LoRa 32 V2** | v3.8 GSM (WiFi + GSM) | `Firmware/flex-fsk-tx-v3.8_GSM/` | WiFi + GSM/LTE failover, REST API, IMAP, MQTT, ChatGPT | RadioLib, U8g2, ArduinoJson, ReadyMail, PubSubClient, RTClib, TinyGSM, SSLClient + `tinyflex/` (Wire/SPI built-in) | ✅ **FULLY SUPPORTED** |
| **Heltec WiFi LoRa 32 V2** | v3.6 WiFi | `Firmware/flex-fsk-tx-v3.6_WiFi/` | WiFi + Web Interface + REST API | RadioLib, U8g2, ArduinoJson, ReadyMail, PubSubClient, RTClib + `tinyflex/` (Wire/SPI built-in) | ✅ **FULLY SUPPORTED** |
| **Heltec WiFi LoRa 32 V2** | v2 | `Firmware/flex-fsk-tx-v2/` | On-device FLEX encoding via AT+MSG | RadioLib, Wire, SPI, U8g2 + `tinyflex/` | ✅ **FULLY SUPPORTED** |
| **Heltec WiFi LoRa 32 V2** | v1 | `Firmware/flex-fsk-tx-v1/` | Basic AT commands, binary transmission | RadioLib, Wire, SPI, U8g2 | ✅ **FULLY SUPPORTED** |

**Current Firmware Versions**: v3.6.92 (WiFi) and v3.8.23 (GSM)

## 🚨 Critical Requirements

### tinyflex Embedded Library Requirement

**IMPORTANT**: v2, v3.6, and v3.8 firmware rely on the bundled `tinyflex/` folder (`#include "tinyflex/tinyflex.h"`). The **entire folder** (or symlink) must sit next to the `.ino` file before compiling.

- Arduino IDE resolves relative includes from the sketch directory
- Each firmware directory already contains symlinks to `../../include/tinyflex` and `../../include/boards`
- No manual copy is required **as long as you keep the repository layout intact**
- If you copy a firmware folder elsewhere (outside the repo) you must copy the entire `tinyflex/` directory into the new location

**Steps when preparing firmware outside this repo**:
1. Navigate to the repository root
2. Copy the full directory for every firmware you export:
   ```bash
   cp -R include/tinyflex "Firmware/flex-fsk-tx-v2/"
   cp -R include/tinyflex "Firmware/flex-fsk-tx-v3.6_WiFi/"
   cp -R include/tinyflex "Firmware/flex-fsk-tx-v3.8_GSM/"
   ```
3. Confirm `tinyflex/tinyflex.h` exists next to the `.ino`
4. Proceed with normal Arduino IDE compilation and upload

**Expected layout (inside the repo)**:
```
Firmware/flex-fsk-tx-v3.6_WiFi/
├── flex-fsk-tx-v3.6_WiFi.ino
├── boards -> ../../include/boards
└── tinyflex -> ../../include/tinyflex
    ├── tinyflex.h
    └── ...

Firmware/flex-fsk-tx-v2/
├── flex-fsk-tx-v2.ino
├── boards -> ../../include/boards
└── tinyflex -> ../../include/tinyflex
    ├── tinyflex.h
    └── ...
```

**Verification**: Open the .ino file in Arduino IDE - compilation should not throw `"tinyflex/tinyflex.h: No such file or directory"`.

> If you archive or relocate a firmware directory outside this repository, copy the actual `include/tinyflex/` (and `include/boards/`) directories into the new location so the includes resolve without the original symlinks.

---

## Firmware Versions

### v1 - Basic AT Commands
- Binary FSK transmission only
- AT command protocol for serial communication
- No FLEX encoding (host must encode)
- Minimal memory footprint

### v2 - On-Device FLEX Encoding
- All v1 features
- On-device FLEX message encoding via `AT+MSG` command
- tinyflex library integration
- Remote encoding support

### v3.6 - WiFi + Web Interface (Recommended)
- All v2 features
- WiFi connectivity with web interface
- REST API with HTTP Basic Auth
- IMAP email-to-pager gateway
- MQTT message queueing
- ChatGPT scheduled prompts
- Message queue (up to 25 messages)
- Remote syslog logging
- Requires `min_spiffs` partition scheme

### v3.8 - WiFi + GSM/Cellular Support
- All v3.6 features and UI/REST stack
- SIM800L and SIMCOM A7670SA modem support (2G/3G/LTE)
- Automatic WiFi/GSM/AP failover and recovery
- Dual-transport MQTT/IMAP/ChatGPT (WiFi or GSM)
- GSM-aware service throttling to preserve bandwidth
- Network health monitoring with display indicators
- GSM pin definitions in `include/boards/boards.h`
- Requires `min_spiffs` partition scheme

**Recommended Firmware**:
- **WiFi-only environments**: v3.6
- **Cellular backup required**: v3.8
- **Host-based encoding**: v1
- **Minimal setup**: v2

---

### TTGO Build Properties Requirement

**IMPORTANT**: TTGO firmware often exceeds the default sketch size limit. You **MUST** use custom build properties to enable compilation.

**Compilation will fail with**: "Sketch too big" or "text section exceeds available space"

**Solution - Use arduino-cli with build properties**:
```bash
# For TTGO v3.6 WiFi firmware (recommended method)
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino

# Or use the flex-build-upload script
OPTIONS="--build-property build.partitions=min_spiffs --build-property upload.maximum_size=1966080" \
  ./scritps/flex-build-upload.sh -t ttgo Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino
```

**Alternative - Modify board configuration** (advanced users):
Edit your ESP32 boards.txt to change default partition scheme for TTGO board to "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)".

**Why this is required**:
- v3 firmware with WiFi, web interface, IMAP, MQTT, and all features is large
- Default partition scheme allocates too little space for application code
- `min_spiffs` partition provides 1966080 bytes (1.9MB) for sketch vs default ~1310720 bytes

### flex-build-upload.sh Automation Script

The repository ships `scritps/flex-build-upload.sh`, a battery-included wrapper around `arduino-cli`. Use it for repeatable builds without hunting for board settings.

- Works from any directory (uses `realpath` for input sketches)
- Automatically selects TTGO/Heltec FQBNs via `-t/--type`
- Adds required build properties (partition size, compile flags)
- `-u/--upload` uploads after compiling and opens a serial monitor if a terminal emulator is available
- `-e/--erase` toggles `EraseFlash=all`
- `-p/--port` selects the serial device (defaults to `/dev/ttyACM0` TTGO or `/dev/ttyUSB0` Heltec)
- Creates timestamped backups keyed by `CURRENT_VERSION` before every build
- Accepts `.bkp-*` files to restore previous firmware versions automatically
- Honors `OPTIONS="--build-property ..."` for advanced overrides

Examples:

```bash
# Compile WiFi firmware only
./scritps/flex-build-upload.sh Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino

# Compile + upload GSM firmware with flash erase on Heltec hardware
./scritps/flex-build-upload.sh -t heltec -u -e \
  Firmware/flex-fsk-tx-v3.8_GSM/flex-fsk-tx-v3.8_GSM.ino
```

> Tip: Run the script from the directory that currently holds your backups or board configs—the script no longer requires changing into the firmware folder first.

## 🔧 Arduino IDE Setup

### 1. Install Arduino IDE

Download and install Arduino IDE 2.x from [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software).

### 2. Add ESP32 Board Support

1. **Open Preferences**: File → Preferences
2. **Add Board Manager URL**:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **Install ESP32 Boards**: Tools → Board → Boards Manager → Search "ESP32" → Install "esp32 by Espressif Systems"

### 3. Install Required Libraries

Use **Tools → Manage Libraries** to install the following libraries:

#### Core Libraries (All Firmwares)
| Library | Author | Purpose | Installation |
|---------|--------|---------|--------------|
| **RadioLib** | Jan Gromeš | LoRa/FSK radio control | Library Manager: Search "RadioLib" |

#### Device-Specific Libraries

**For TTGO LoRa32-OLED firmwares**:
| Library | Author | Purpose | Installation |
|---------|--------|---------|--------------|
| **U8g2** | oliver | OLED display control | Library Manager: Search "U8g2" |

**For Heltec WiFi LoRa 32 V2 firmwares**:
| Library | Author | Purpose | Installation |
|---------|--------|---------|--------------|
| **U8g2** | oliver | OLED display control | Library Manager: Search "U8g2" |
| **Wire** | ESP32 Core | I2C communication | Built-in (no installation) |
| **SPI** | ESP32 Core | SPI communication | Built-in (no installation) |

#### Advanced Features (v3.6/v3.8 Firmware Only)
| Library | Author | Purpose | Installation |
|---------|--------|---------|--------------|
| **ArduinoJson** | Benoit Blanchon | JSON handling for REST API | Library Manager: Search "ArduinoJson" |
| **ReadyMail** | Khoi Hoang | IMAP email client for email-to-pager | Library Manager: Search "ReadyMail" or "ESP Mail Client" |
| **PubSubClient** | Nick O'Leary | MQTT client for bidirectional messaging | Library Manager: Search "PubSubClient" |
| **RTClib** | Adafruit | DS3231 RTC support (optional) | Library Manager: Search "RTClib" |

#### GSM/Cellular Features (v3.8 Firmware Only)
| Library | Author | Purpose | Installation |
|---------|--------|---------|--------------|
| **TinyGsmClient** | Volodymyr Shymanskyy | GSM/GPRS modem support (SIM800L) | Library Manager: Search "TinyGSM" |
| **SSLClient** | OPEnSLab-OSU | TLS/SSL over GSM for secure MQTT | Library Manager: Search "SSLClient" |

**Note**: v3.8 firmware requires all v3.6 WiFi libraries (including RTClib) plus the GSM libraries above.

**Note**: Board pin definitions are included locally in `boards/boards.h` within each firmware directory. No external board library needed.

### 4. Verify Library Installation

**Check installed libraries**: Tools → Manage Libraries → Filter "Installed"

**Expected libraries for TTGO v3.6 firmware**:
- ✅ RadioLib
- ✅ U8g2
- ✅ ArduinoJson
- ✅ ReadyMail (or ESP Mail Client)
- ✅ PubSubClient
- ✅ RTClib (if RTC_ENABLED=true)

**Expected libraries for Heltec V2 v3.6 firmware**:
- ✅ RadioLib
- ✅ U8g2
- ✅ ArduinoJson
- ✅ ReadyMail (or ESP Mail Client)
- ✅ PubSubClient
- ✅ RTClib (if RTC_ENABLED=true)
- ✅ Wire (built-in)
- ✅ SPI (built-in)

**Expected libraries for v3.8 firmware (TTGO or Heltec V2)**:
- ✅ All v3 libraries above, plus:
- ✅ TinyGsmClient (TinyGSM)
- ✅ SSLClient

## 📱 Device-Specific Flashing Procedures

### TTGO LoRa32-OLED Flashing

#### Hardware Specifications
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio**: SX1276 (137-1020 MHz)
- **Power**: 2-20 dBm transmit power
- **Display**: 0.96" OLED (128x64)
- **Serial Port**: `/dev/ttyACM0` (Linux), `COM3+` (Windows)
- **Default Frequency**: 915.0 MHz
- **Message Length**: Up to 248 characters

#### Hardware Preparation
1. **Connect USB cable** to TTGO device and computer
2. **Install appropriate antenna** for your frequency band
3. **Check device detection**:
   ```bash
   # Linux/macOS
   ls /dev/tty*
   # Look for /dev/ttyACM0 or similar

   # Windows
   # Check Device Manager → Ports (COM & LPT)
   ```

#### Board Configuration
1. **Select Board**: Tools → Board → ESP32 Arduino → "TTGO LoRa32-OLED V1"
   - Alternative: "ESP32 Dev Module" if TTGO option unavailable
2. **Configure Settings**:
   - **Upload Speed**: 921600 (or 115200 if upload fails)
   - **CPU Frequency**: 240MHz (WiFi/BT)
   - **Flash Frequency**: 80MHz
   - **Flash Mode**: QIO
   - **Flash Size**: 4MB (32Mb)
   - **Partition Scheme**: Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS) - **REQUIRED for v3**
   - **Core Debug Level**: None
   - **Port**: Select your device port (e.g., /dev/ttyACM0, COM3)

#### Firmware Selection and Flashing

**v3.6 WiFi Firmware (Web Interface + REST)**:
```bash
# 1. (Only if you moved this folder) Recreate tinyflex beside the .ino
[ -d Firmware/flex-fsk-tx-v3.6_WiFi/tinyflex ] || \
  cp -R include/tinyflex Firmware/flex-fsk-tx-v3.6_WiFi/

# 2. Open firmware in Arduino IDE
# File → Open → Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino

# 3. Verify libraries installed via Library Manager:
# - RadioLib, U8g2, ArduinoJson
# - ReadyMail, PubSubClient (for v3 IMAP/MQTT features)

# 4. CRITICAL: Set Partition Scheme to "Minimal SPIFFS"

# 5. Upload firmware: Sketch → Upload
# OR use arduino-cli with build properties (recommended):
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino

# Or use the flex-build-upload script (runs from any directory):
./scritps/flex-build-upload.sh -t ttgo \
  Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino
```

**v3.8 Firmware (WiFi + GSM/Cellular)**:
```bash
# 1. (Only if you moved this folder) Recreate tinyflex beside the .ino
[ -d Firmware/flex-fsk-tx-v3.8_GSM/tinyflex ] || \
  cp -R include/tinyflex Firmware/flex-fsk-tx-v3.8_GSM/

# 2. Open firmware in Arduino IDE
# File → Open → Firmware/flex-fsk-tx-v3.8_GSM/flex-fsk-tx-v3.8_GSM.ino

# 3. Verify libraries installed:
# - RadioLib, U8g2, ArduinoJson, ReadyMail, PubSubClient, RTClib
# - TinyGSM, SSLClient (for GSM transport)

# 4. Set Partition Scheme to "Minimal SPIFFS"

# 5. Upload firmware or use the build script:
./scritps/flex-build-upload.sh -t ttgo -u -e \
  Firmware/flex-fsk-tx-v3.8_GSM/flex-fsk-tx-v3.8_GSM.ino
```

**v2 Firmware (On-device FLEX encoding)**:
```bash
# 1. (Only if you moved this folder) Recreate tinyflex beside the .ino
[ -d Firmware/flex-fsk-tx-v2/tinyflex ] || \
  cp -R include/tinyflex Firmware/flex-fsk-tx-v2/

# 2. Open firmware
# File → Open → Firmware/flex-fsk-tx-v2/flex-fsk-tx-v2.ino

# 3. Upload firmware
```

**v1 Firmware (Basic AT commands)**:
```bash
# 1. tinyflex folder NOT required for v1
# 2. Open firmware
# File → Open → Firmware/flex-fsk-tx-v1/flex-fsk-tx-v1.ino

# 3. Upload firmware
```

#### Upload Troubleshooting (TTGO)
**Upload fails with "Failed to connect"**:
1. Hold **BOOT** button on device
2. Press **RST** button briefly
3. Release **RST** button
4. Release **BOOT** button
5. Click Upload in Arduino IDE immediately

**"Sketch too big" error**:
- Use Minimal SPIFFS partition scheme (see Board Configuration above)
- Or use arduino-cli with build properties (see v3 firmware commands)

**Upload speed issues**:
- Try lower upload speed: 115200 or 460800
- Use different USB cable (data cable, not charging only)
- Try different USB port

### Heltec WiFi LoRa 32 V2 Flashing

#### Hardware Specifications
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio**: SX1276 (137-1020 MHz)
- **Power**: 2-20 dBm transmit power
- **Display**: 0.96" OLED (128x64)
- **Serial Port**: `/dev/ttyUSB0` (Linux), `COM4+` (Windows)
- **Default Frequency**: 915.0 MHz
- **Message Length**: Up to 248 characters

#### Hardware Preparation
1. **Connect USB cable** to Heltec device and computer
2. **Install appropriate antenna** for your frequency band
3. **Check device detection**:
   ```bash
   # Linux/macOS - usually shows as USB-to-UART bridge
   ls /dev/tty*
   # Look for /dev/ttyUSB0 or similar

   # Windows
   # Check Device Manager for CP210x or CH340 bridge
   ```

#### Board Configuration
1. **Select Board**: Tools → Board → ESP32 Arduino → "ESP32 Dev Module"
   - **Note**: Heltec V2 uses generic ESP32 board selection (not V3-specific board)
2. **Configure Settings**:
   - **Upload Speed**: 921600 (or 115200 if upload fails)
   - **CPU Frequency**: 240MHz (WiFi/BT)
   - **Flash Frequency**: 80MHz
   - **Flash Mode**: QIO
   - **Flash Size**: 4MB (32Mb)
   - **Partition Scheme**: Default 4MB with spiffs (or Minimal SPIFFS for v3)
   - **Core Debug Level**: None
   - **Port**: Select your device port (e.g., /dev/ttyUSB0, COM4)

#### Firmware Selection and Flashing

**v3.6 Firmware (WiFi + Web Interface)**:
```bash
# 1. (Only if you moved this folder) Recreate tinyflex beside the .ino
[ -d Firmware/flex-fsk-tx-v3.6_WiFi/tinyflex ] || \
  cp -R include/tinyflex Firmware/flex-fsk-tx-v3.6_WiFi/

# 2. Open firmware in Arduino IDE
# File → Open → Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino

# 3. Verify libraries installed:
# - RadioLib, U8g2, ArduinoJson, Wire (built-in), SPI (built-in)

# 4. Upload firmware: Sketch → Upload
```

**v2 Firmware (On-device FLEX encoding)**:
```bash
# 1. (Only if you moved this folder) Recreate tinyflex beside the .ino
[ -d Firmware/flex-fsk-tx-v2/tinyflex ] || \
  cp -R include/tinyflex Firmware/flex-fsk-tx-v2/

# 2. Open firmware
# File → Open → Firmware/flex-fsk-tx-v2/flex-fsk-tx-v2.ino

# 3. Upload firmware
```

**v1 Firmware (Basic AT commands)**:
```bash
# 1. tinyflex folder NOT required for v1
# 2. Open firmware
# File → Open → Firmware/flex-fsk-tx-v1/flex-fsk-tx-v1.ino

# 3. Upload firmware
```

**v3.8 Firmware (WiFi + GSM/Cellular Support)**:
```bash
# 1. (Only if you moved this folder) Recreate tinyflex beside the .ino
[ -d Firmware/flex-fsk-tx-v3.8_GSM/tinyflex ] || \
  cp -R include/tinyflex Firmware/flex-fsk-tx-v3.8_GSM/

# 2. Open firmware in Arduino IDE
# File → Open → Firmware/flex-fsk-tx-v3.8_GSM/flex-fsk-tx-v3.8_GSM.ino

# 3. Install ALL required libraries via Library Manager:
# Core libraries (see section 3 above):
# - RadioLib, U8g2, ArduinoJson
# - ReadyMail (or ESP Mail Client), PubSubClient
# GSM-specific libraries:
# - TinyGSM (search "TinyGSM")
# - SSLClient (search "SSLClient")

# 4. Upload firmware
```

#### Upload Troubleshooting (Heltec V2)
**Upload fails or device not detected**:
1. Install CP210x or CH340 USB drivers if needed
2. Try different USB cable
3. Hold **PRG** button during upload process
4. Check Windows Device Manager for driver issues

**Compilation errors**:
- **Missing library errors**: Ensure all required libraries are installed (see section 3)
-  - v1/v2: RadioLib, U8g2
-  - v3.6: Add ArduinoJson, ReadyMail, PubSubClient
-  - v3.8: Add TinyGSM, SSLClient (plus all v3.6 libraries)
- **tinyflex folder missing**: Copy `include/tinyflex` into the firmware directory so `tinyflex/tinyflex.h` exists beside the `.ino`
- **boards.h not found**: Ensure local `boards/boards.h` exists in firmware directory
- Try Arduino IDE restart after library installation
- Verify board selection matches your hardware

## 🔍 Verification and Testing

### 1. Upload Success Verification

**Check Serial Monitor**:
1. **Open Serial Monitor**: Tools → Serial Monitor
2. **Set Baud Rate**: 115200
3. **Set Line Ending**: Both NL & CR
4. **Device should display**: Banner and "Ready" status

**Expected startup messages**:
```
flex-fsk-tx
Initializing...
LoRa init: OK
Display init: OK
Ready for AT commands
```

### 2. Basic AT Command Testing

**Test basic connectivity**:
```bash
# Send AT command in Serial Monitor
AT
# Expected response: OK

# Check device status
AT+STATUS?
# Expected response: +STATUS: READY

# Test frequency setting
AT+FREQ=929.6625
# Expected response: OK
```

### 3. Firmware Version Verification

**v1 Firmware - Basic Commands**:
```bash
AT+SEND=10        # Should respond: +SEND: READY
AT+MSG=1234567    # Should respond: ERROR (not supported)
```

**v2 Firmware - FLEX Encoding Support**:
```bash
AT+MSG=1234567    # Should respond: +MSG: READY
AT+WIFI=test,pass # Should respond: ERROR (not supported)
```

**v3 Firmware - Full WiFi Support (v3.6)**:
```bash
AT+VERSION?       # Should respond: +VERSION: v3.6
AT+MSG=1234567    # Should respond: +MSG: READY
AT+WIFI?          # Should respond: +WIFI: DISCONNECTED
AT+APIUSER?       # Should respond: +APIUSER: username
```

### 4. OLED Display Verification

**Check display shows**:
- **Line 1**: "flex-fsk-tx" banner or device branding
- **Line 2**: Current status (Ready, Transmitting, etc.)
- **Line 3**: Frequency setting
- **Line 4**: Power setting

**v3 Firmware specific**:
- **WiFi Status**: Connected/Disconnected + IP address
- **Battery Status**: Percentage and power state (if applicable)
- **API Status**: Port number (on status page)

### 5. v3.6 Feature Testing

**Test Enhanced Features**:
```bash
# Test PPM correction precision (now 0.02 decimals)
AT+PPM=1.23
# Expected response: OK

# Test watchdog operations validation
# Device should be stable without unexpected resets

# Monitor serial for watchdog task registration logs
# Should see proper task registration on boot
```

## 🚨 Troubleshooting

**🔧 Complete Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive firmware and hardware issue resolution.

### Quick Firmware Issues

### Compilation Errors

**"tinyflex/tinyflex.h: No such file or directory"**:
```bash
# Solution: Ensure the tinyflex folder (or symlink) lives next to the .ino
ls -l Firmware/[VERSION]/tinyflex

# Inside this repo you can recreate the symlink if needed:
ln -s ../../include/tinyflex Firmware/[VERSION]/tinyflex

# If you exported the firmware elsewhere, copy the folder:
[ -d Firmware/[VERSION]/tinyflex ] || \
  cp -R include/tinyflex Firmware/[VERSION]/
```

**"U8g2lib.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "U8g2" → Install
```

**"ArduinoJson.h: No such file or directory" (v3 firmware)**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "ArduinoJson" → Install
```

**"ReadyMail.h: No such file or directory" (v3.6/v3.8 firmware)**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "ReadyMail" or "ESP Mail Client" → Install
```

**"PubSubClient.h: No such file or directory" (v3.6/v3.8 firmware)**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "PubSubClient" → Install
```

**"TinyGsmClient.h: No such file or directory" (v3.8 firmware)**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "TinyGSM" → Install
```

**"SSLClient.h: No such file or directory" (v3.8 firmware)**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "SSLClient" → Install
```

**"Sketch too big" or "text section exceeds available space" (TTGO v3.6/v3.8)**:
```bash
# Solution 1: Use Minimal SPIFFS partition in Arduino IDE
# Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)

# Solution 2: Use arduino-cli with build properties
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  Firmware/flex-fsk-tx-v3.6_WiFi/flex-fsk-tx-v3.6_WiFi.ino
```

### Upload Errors

**ESP32 not detected or upload fails**:
1. **Check USB drivers**:
   - **TTGO**: Usually uses CH340 or CP210x
   - **Heltec V2**: Usually uses CP210x
2. **Try manual upload mode**:
   - Hold BOOT/PRG button
   - Press RESET briefly
   - Start upload
   - Release BOOT/PRG when upload begins
3. **Lower upload speed**: Change to 115200 in Tools → Upload Speed

**"A fatal error occurred: Failed to connect to ESP32"**:
- Try different USB cable (ensure data capability)
- Check port selection in Tools → Port
- Restart Arduino IDE
- Try different USB port

### Runtime Errors

**Device starts but AT commands not working**:
1. **Check baud rate**: Must be 115200
2. **Check line endings**: Set to "Both NL & CR"
3. **Try basic AT command**: Just "AT" without parameters

**OLED display blank or garbled**:
- **TTGO**: Verify U8g2 library installed (RadioLib is common to all)
- **Heltec V2**: Verify U8g2, Wire, and SPI libraries available
- Check device power (USB or battery)
- Try firmware re-upload

**WiFi features not working (v3.6/v3.8 firmware)**:
```bash
# Check if WiFi AT commands are recognized
AT+WIFI?
# If returns ERROR, verify you flashed v3.6/v3.8 firmware

# Check ArduinoJson library installation
# Tools → Manage Libraries → Installed → Search "ArduinoJson"
```

### Device-Specific Issues

**TTGO LoRa32-OLED**:
- **Serial port**: Usually `/dev/ttyACM0` on Linux, `COM3+` on Windows
- **Upload mode**: May require BOOT+RESET button sequence
- **Board selection**: "TTGO LoRa32-OLED V1" or "ESP32 Dev Module"
- **Partition scheme**: Must use Minimal SPIFFS for v3.6/v3.8 firmware

**Heltec WiFi LoRa 32 V2**:
- **Serial port**: Usually `/dev/ttyUSB0` on Linux, `COM4+` on Windows
- **Upload mode**: Usually automatic, may need PRG button
- **Board selection**: Must be "ESP32 Dev Module"
- **Radio chipset**: SX1276 (same as TTGO, full 248 character support)

## 📋 Pre-Flash Checklist

Before flashing any firmware, verify:

- [ ] **Arduino IDE installed** with ESP32 board support
- [ ] **Device detected** and proper port selected
- [ ] **Required libraries installed** (see tables above)
- [ ] **tinyflex folder present** next to the `.ino` (v2/v3.6/v3.8 only)
- [ ] **Proper board selected** for your hardware
- [ ] **Partition scheme set** (Minimal SPIFFS for TTGO v3.6/v3.8)
- [ ] **USB cable supports data** (not just charging)
- [ ] **Antenna connected** to device

## 📚 Related Documentation

For usage after successful firmware installation:
- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface setup and usage (v3.6/v3.8 firmware)
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference
- **[REST_API.md](REST_API.md)**: REST API documentation (v3.6/v3.8 firmware)
- **[README.md](../README.md)**: Project overview and quick start examples

## 🆘 Getting Help

**🔧 Comprehensive Issue Resolution**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for:
- Complete firmware troubleshooting procedures
- Arduino IDE compilation and upload error resolution
- Hardware debugging and recovery procedures
- Professional GitHub issue reporting templates
- Emergency recovery procedures

### Quick Firmware Help
1. **Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)** for detailed firmware issue resolution
2. **Verify all requirements** met per Pre-Flash Checklist above
3. **Try emergency recovery** procedures if device unresponsive
4. **Follow GitHub issue templates** for professional problem reporting

---

**Firmware installation complete!** Once flashed successfully, your device is ready for FLEX message transmission. See the related documentation above for usage instructions.
