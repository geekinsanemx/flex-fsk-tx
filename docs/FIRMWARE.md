# FLEX Paging Message Transmitter - Firmware Installation Guide

Complete guide for flashing firmware to ESP32 LoRa32 devices for FLEX paging transmission.

## 🎯 Quick Reference

| Device | Firmware | Key Features | Libraries Required | Status |
|--------|----------|--------------|-------------------|--------|
| **TTGO LoRa32-OLED** | v3 | WiFi + Web Interface + REST API | RadioLib, RadioBoards, U8g2, ArduinoJson + tinyflex.h | ✅ **FULLY SUPPORTED** |
| **TTGO LoRa32-OLED** | v2 | On-device FLEX encoding via AT+MSG | RadioLib, RadioBoards, U8g2 + tinyflex.h | ✅ **FULLY SUPPORTED** |
| **TTGO LoRa32-OLED** | v1 | Basic AT commands, binary transmission | RadioLib, RadioBoards, U8g2 | ✅ **FULLY SUPPORTED** |
| **Heltec WiFi LoRa 32 V2** | v3 | WiFi + Web Interface + REST API | RadioLib, Wire, SPI, U8g2, ArduinoJson + tinyflex.h | ✅ **FULLY SUPPORTED** |
| **Heltec WiFi LoRa 32 V2** | v2 | On-device FLEX encoding via AT+MSG | RadioLib, Wire, SPI, U8g2 + tinyflex.h | ✅ **FULLY SUPPORTED** |
| **Heltec WiFi LoRa 32 V2** | v1 | Basic AT commands, binary transmission | RadioLib, Wire, SPI, U8g2 | ✅ **FULLY SUPPORTED** |

**Current Firmware Version**: v3.6.68

## 🚨 Critical Requirements

### tinyflex.h Embedding Requirement

**IMPORTANT**: For v2 and v3 firmware versions that support on-device FLEX encoding (AT+MSG command), you **MUST** manually copy the `tinyflex.h` file into the same directory as the .ino firmware file before compiling.

**Why this is required**:
- v2 and v3 firmware include `#include "tinyflex.h"` with quotes (local include)
- Arduino IDE looks for local includes in the same directory as the .ino file
- The tinyflex library provides FLEX protocol encoding capabilities on the ESP32 device

**Steps for every v2/v3 firmware flash**:
1. Navigate to your firmware directory
2. Copy `tinyflex.h` to the firmware version directory:
   ```bash
   # For TTGO v3 firmware
   cp include/tinyflex/tinyflex.h "Firmware/v3/"

   # For TTGO v2 firmware
   cp include/tinyflex/tinyflex.h "Firmware/v2/"

   # For Heltec V2 v3 firmware
   cp include/tinyflex/tinyflex.h "Firmware/v3/"

   # For Heltec V2 v2 firmware
   cp include/tinyflex/tinyflex.h "Firmware/v2/"
   ```
3. The firmware directory should contain both the .ino file and tinyflex.h
4. Proceed with normal Arduino IDE compilation and upload

**File structure after copying**:
```
Firmware/v3/
├── flex_fsk_tx-v3.ino     # Main firmware file
├── tinyflex.h                # REQUIRED: Copied from include/tinyflex/
└── (other files...)

Firmware/v3/
├── flex_fsk_tx-v3.ino   # Main firmware file
├── tinyflex.h                # REQUIRED: Copied from include/tinyflex/
└── (other files...)
```

**Verification**: Open the .ino file in Arduino IDE - it should compile without "tinyflex.h not found" errors.

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
- tinyflex.h library integration
- Remote encoding support

### v3 - WiFi + Web Interface (Recommended)
- All v2 features
- WiFi connectivity with web interface
- REST API with HTTP Basic Auth
- IMAP email-to-pager gateway
- MQTT message queueing
- ChatGPT scheduled prompts
- Message queue (up to 25 messages)
- Remote syslog logging
- Requires `min_spiffs` partition scheme

### v4 - GSM/2G Cellular Support
- All v3 features
- SIM800L GSM module support (2G/GPRS)
- Automatic WiFi/GSM failover
- Dual-transport MQTT/IMAP (WiFi or GSM)
- Network health monitoring
- Automatic WiFi recovery attempts
- GSM pin definitions in `include/boards/boards.h`
- Requires `min_spiffs` partition scheme

**Recommended Firmware**:
- **WiFi-only environments**: v3
- **Cellular backup required**: v4
- **Host-based encoding**: v1
- **Minimal setup**: v2

---

### TTGO Build Properties Requirement

**IMPORTANT**: TTGO firmware often exceeds the default sketch size limit. You **MUST** use custom build properties to enable compilation.

**Compilation will fail with**: "Sketch too big" or "text section exceeds available space"

**Solution - Use arduino-cli with build properties**:
```bash
# For TTGO v3 firmware (recommended method)
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  Firmware/v3/flex_fsk_tx-v3.ino

# Or use the project build script (if available)
OPTIONS="--build-properties build.partitions=min_spiffs,upload.maximum_size=1966080" \
  ttgo-build-upload.sh Firmware/v3/flex_fsk_tx-v3.ino
```

**Alternative - Modify board configuration** (advanced users):
Edit your ESP32 boards.txt to change default partition scheme for TTGO board to "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)".

**Why this is required**:
- v3 firmware with WiFi, web interface, IMAP, MQTT, and all features is large
- Default partition scheme allocates too little space for application code
- `min_spiffs` partition provides 1966080 bytes (1.9MB) for sketch vs default ~1310720 bytes

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
| **RadioBoards** | radiolib-org | TTGO board definitions | Manual installation (see below) |

**For Heltec WiFi LoRa 32 V2 firmwares**:
| Library | Author | Purpose | Installation |
|---------|--------|---------|--------------|
| **U8g2** | oliver | OLED display control | Library Manager: Search "U8g2" |
| **Wire** | ESP32 Core | I2C communication | Built-in (no installation) |
| **SPI** | ESP32 Core | SPI communication | Built-in (no installation) |

#### Advanced Features (v3 Firmware Only)
| Library | Author | Purpose | Installation |
|---------|--------|---------|--------------|
| **ArduinoJson** | Benoit Blanchon | JSON handling for REST API | Library Manager: Search "ArduinoJson" |

### 4. Manual RadioBoards Installation

**Required for TTGO devices only**:

```bash
# Navigate to Arduino libraries directory
cd ~/Arduino/libraries/

# Clone RadioBoards library
git clone https://github.com/radiolib-org/RadioBoards.git

# Restart Arduino IDE to recognize the library
```

**Alternative manual installation**:
1. Download ZIP from [https://github.com/radiolib-org/RadioBoards](https://github.com/radiolib-org/RadioBoards)
2. Extract to `~/Arduino/libraries/RadioBoards/`
3. Restart Arduino IDE

### 5. Verify Library Installation

**Check installed libraries**: Tools → Manage Libraries → Filter "Installed"

**Expected libraries for TTGO v3 firmware**:
- ✅ RadioLib
- ✅ U8g2
- ✅ ArduinoJson
- ✅ RadioBoards (manual installation)

**Expected libraries for Heltec V2 v3 firmware**:
- ✅ RadioLib
- ✅ U8g2
- ✅ ArduinoJson
- ✅ Wire (built-in)
- ✅ SPI (built-in)

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

**v3 Firmware (WiFi + Web Interface) - Current Version v3.6.68**:
```bash
# 1. Copy tinyflex.h (REQUIRED)
cp include/tinyflex/tinyflex.h "Firmware/v3/"

# 2. Open firmware in Arduino IDE
# File → Open → Firmware/v3/flex_fsk_tx-v3.ino

# 3. Verify libraries installed:
# - RadioLib, U8g2, ArduinoJson, RadioBoards

# 4. CRITICAL: Set Partition Scheme to "Minimal SPIFFS"

# 5. Upload firmware: Sketch → Upload
# OR use arduino-cli with build properties (recommended):
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  Firmware/v3/flex_fsk_tx-v3.ino
```

**v2 Firmware (On-device FLEX encoding)**:
```bash
# 1. Copy tinyflex.h (REQUIRED)
cp include/tinyflex/tinyflex.h "Firmware/v2/"

# 2. Open firmware
# File → Open → Firmware/v2/flex_fsk_tx_v2.ino

# 3. Upload firmware
```

**v1 Firmware (Basic AT commands)**:
```bash
# 1. tinyflex.h NOT required for v1
# 2. Open firmware
# File → Open → Firmware/v1/flex_fsk_tx_v1.ino

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

**v3 Firmware (WiFi + Web Interface) - Current Version v3.6.68**:
```bash
# 1. Copy tinyflex.h (REQUIRED)
cp include/tinyflex/tinyflex.h "Firmware/v3/"

# 2. Open firmware in Arduino IDE
# File → Open → Firmware/v3/flex_fsk_tx-v3.ino

# 3. Verify libraries installed:
# - RadioLib, U8g2, ArduinoJson, Wire (built-in), SPI (built-in)

# 4. Upload firmware: Sketch → Upload
```

**v2 Firmware (On-device FLEX encoding)**:
```bash
# 1. Copy tinyflex.h (REQUIRED)
cp include/tinyflex/tinyflex.h "Firmware/v2/"

# 2. Open firmware
# File → Open → Firmware/v2/flex_fsk_tx_v2.ino

# 3. Upload firmware
```

**v1 Firmware (Basic AT commands)**:
```bash
# 1. tinyflex.h NOT required for v1
# 2. Open firmware
# File → Open → Firmware/v1/flex_fsk_tx_v1.ino

# 3. Upload firmware
```

**v4 Firmware (WiFi + GSM/2G Support)**:
```bash
# 1. Copy tinyflex.h (REQUIRED)
cp include/tinyflex/tinyflex.h "Firmware/v4/"

# 2. Open firmware in Arduino IDE
# File → Open → Firmware/v4/flex_fsk_tx-v4.ino

# 3. Install additional libraries:
# - TinyGSM (for SIM800L support)
# - SSLClient (for GSM TLS/SSL)

# 4. Upload firmware
```

#### Upload Troubleshooting (Heltec V2)
**Upload fails or device not detected**:
1. Install CP210x or CH340 USB drivers if needed
2. Try different USB cable
3. Hold **PRG** button during upload process
4. Check Windows Device Manager for driver issues

**Compilation errors**:
- Ensure U8g2, Wire, and SPI libraries are available
- Try Arduino IDE restart after library installation
- Verify board selection is "ESP32 Dev Module"

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

**v3 Firmware - Full WiFi Support (v3.6.68)**:
```bash
AT+VERSION?       # Should respond: +VERSION: v3.6.68
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

### 5. v3.6.68 Feature Testing

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

**"tinyflex.h: No such file or directory"**:
```bash
# Solution: Copy tinyflex.h to firmware directory
cp include/tinyflex/tinyflex.h "Firmware/[VERSION]/"
```

**"RadioBoards.h: No such file or directory" (TTGO only)**:
```bash
# Solution: Install RadioBoards library manually
cd ~/Arduino/libraries/
git clone https://github.com/radiolib-org/RadioBoards.git
# Restart Arduino IDE
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

**"Sketch too big" or "text section exceeds available space" (TTGO v3)**:
```bash
# Solution 1: Use Minimal SPIFFS partition in Arduino IDE
# Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)

# Solution 2: Use arduino-cli with build properties
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  Firmware/v3/flex_fsk_tx-v3.ino
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
- **TTGO**: Verify U8g2 and RadioBoards libraries installed
- **Heltec V2**: Verify U8g2, Wire, and SPI libraries available
- Check device power (USB or battery)
- Try firmware re-upload

**WiFi features not working (v3 firmware)**:
```bash
# Check if WiFi AT commands are recognized
AT+WIFI?
# If returns ERROR, verify you flashed v3 firmware

# Check ArduinoJson library installation
# Tools → Manage Libraries → Installed → Search "ArduinoJson"
```

### Device-Specific Issues

**TTGO LoRa32-OLED**:
- **Serial port**: Usually `/dev/ttyACM0` on Linux, `COM3+` on Windows
- **Upload mode**: May require BOOT+RESET button sequence
- **Board selection**: "TTGO LoRa32-OLED V1" or "ESP32 Dev Module"
- **Partition scheme**: Must use Minimal SPIFFS for v3 firmware

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
- [ ] **tinyflex.h copied** to firmware directory (v2/v3 only)
- [ ] **Proper board selected** for your hardware
- [ ] **Partition scheme set** (Minimal SPIFFS for TTGO v3)
- [ ] **USB cable supports data** (not just charging)
- [ ] **Antenna connected** to device

## 📚 Related Documentation

For usage after successful firmware installation:
- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface setup and usage (v3 firmware)
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference
- **[REST_API.md](REST_API.md)**: REST API documentation (v3 firmware)
- **[README.md](README.md)**: Project overview and quick start examples

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
