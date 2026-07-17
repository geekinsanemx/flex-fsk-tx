# FLEX Paging Message Transmitter - Firmware Installation Guide

Complete guide for flashing firmware to ESP32 LoRa32 devices for FLEX paging transmission.

This is single-variant firmware (`flex-fsk-tx-v2.ino` at the repository root) — every
feature described in this guide (WiFi, web interface, REST API, IMAP, MQTT, ChatGPT,
GSM/cellular failover) is present in the same build. There is no separate AT-only or
WiFi-only firmware image to choose between; board selection and a small set of
compile-time feature flags (below) are the only build-time choices.

## 🎯 Quick Reference

| Device | MCU | Radio | Display | Serial Port (Linux) | Power | Message Length |
|--------|-----|-------|---------|----------------------|-------|-----------------|
| **TTGO LoRa32-OLED** | ESP32 (240MHz dual-core) | SX1276 (137-1020 MHz) | 0.96" OLED (128x64) | `/dev/ttyACM0` | -9 to 20 dBm | Up to 248 characters |
| **Heltec WiFi LoRa 32 V2** | ESP32 (240MHz dual-core) | SX1276 (137-1020 MHz) | 0.96" OLED (128x64) | `/dev/ttyUSB0` | -9 to 20 dBm | Up to 248 characters |

**Default transmit frequency**: 931.9375 MHz (`TX_FREQ_DEFAULT` in `config.h`, same default
on both boards).

**Board selection** is a compile-time flag, not a separate firmware file — see
[Board Selection](#board-selection) below.

## 🚨 Critical Requirements

### tinyflex Embedded Library Requirement

**IMPORTANT**: the firmware includes the bundled tinyflex library directly:
`#include "include/tinyflex/tinyflex.h"`. Since the sketch (`flex-fsk-tx-v2.ino`) lives at
the repository root and `include/tinyflex/` is a real subdirectory of the same repo (not a
symlink), this resolves automatically — **no setup step is required** as long as you keep
the repository layout intact.

**If you copy the sketch elsewhere** (outside this repository), you must also copy the
`include/` directory (specifically `include/tinyflex/` and `include/boards/`) to the same
relative location next to the `.ino` file, since Arduino IDE resolves relative includes
from the sketch directory.

**Verification**: Open `flex-fsk-tx-v2.ino` in Arduino IDE — compilation should not throw
`"include/tinyflex/tinyflex.h: No such file or directory"`.

### Board Selection

Board selection is controlled by a compile-time macro in `config.h`, **not** by editing
the `.ino` file or choosing a different sketch:

```cpp
// config.h
#if !defined(TTGO_LORA32_V21) && !defined(HELTEC_WIFI_LORA32_V2)
  #define TTGO_LORA32_V21
#endif
```

- Defaults to **TTGO_LORA32_V21** if neither macro is defined.
- To build for **Heltec WiFi LoRa 32 V2**, pass the macro at compile time instead of
  editing `config.h`:
  ```bash
  arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V2 \
    --build-property "compiler.cpp.extra_flags=-DHELTEC_WIFI_LORA32_V2" \
    flex-fsk-tx-v2.ino
  ```
- `scripts/flex-build-upload.sh -t heltec` does this for you automatically — see
  [flex-build-upload.sh Automation Script](#flex-build-uploadsh-automation-script) below.

### Compile-Time Feature Flags (config.h)

`RTC`, `IMAP`, `ChatGPT`, and `GSM` are optional subsystems that are **disabled by default**
— `config.h` does not `#define` any of them. Each is opted into individually via a bare
compiler command-line define, existence-checked in code (`#ifdef`, not a value comparison):

| Flag | Default | Effect |
|------|---------|--------|
| `ENABLE_RTC` | not defined (off) | Compiles in RTClib/DS3231 RTC support for immediate boot timestamps. |
| `ENABLE_IMAP` | not defined (off) | Compiles in ReadyMail and IMAP email-to-page polling. |
| `ENABLE_CHATGPT` | not defined (off) | Compiles in scheduled ChatGPT prompt execution. |
| `ENABLE_GSM` | not defined (off) | Compiles in TinyGSM/SSLClient and GSM/cellular failover support. |
| `ENABLE_DEBUG` | defined (on) | Verbose serial debug output. |

`./scripts/flex-build-upload.sh`'s `--enable-rtc` / `--enable-imap` / `--enable-chatgpt` /
`--enable-gsm` flags set the matching `-DENABLE_*` define automatically — see
[flex-build-upload.sh Automation Script](#flex-build-uploadsh-automation-script) below. To
set these manually with plain `arduino-cli`, pass e.g.
`--build-property "compiler.cpp.extra_flags=-DENABLE_GSM"`.

---

### TTGO Build Properties Requirement

**IMPORTANT**: the full feature set (WiFi, web interface, REST API, IMAP, MQTT, GSM) makes
the firmware large enough that TTGO's default partition scheme runs out of space.

**Compilation will fail with**: "Sketch too big" or "text section exceeds available space"

**Solution - Use arduino-cli with build properties**:
```bash
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  flex-fsk-tx-v2.ino

# Or use the flex-build-upload script (applies these automatically for -t ttgo)
./scripts/flex-build-upload.sh -t ttgo flex-fsk-tx-v2.ino
```

**Alternative - Modify board configuration** (advanced users, Arduino IDE GUI):
Tools → Partition Scheme → "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)".

**Why this is required**:
- Default partition scheme allocates too little space for application code
- `min_spiffs` partition provides 1,966,080 bytes (1.9MB) for the sketch vs. the default
  ~1,310,720 bytes
- Heltec WiFi LoRa 32 V2's default partition scheme has enough headroom without this
  change — `flex-build-upload.sh -t heltec` does not apply it

### flex-build-upload.sh Automation Script

The repository ships `scripts/flex-build-upload.sh`, a battery-included wrapper around
`arduino-cli`. Use it for repeatable builds without hunting for board settings.

- Works from any directory (uses `realpath` for the input sketch path)
- Automatically selects TTGO/Heltec FQBNs and build properties via `-t/--type`
- `--enable-rtc` / `--enable-imap` / `--enable-chatgpt` / `--enable-gsm` opt individual
  optional subsystems into the build (all four are off by default)
- Before compiling, automatically checks that `arduino-cli` has the `esp32:esp32` board
  core and every Arduino library required by the requested `--enable-*` flags installed,
  and installs anything missing (`arduino-cli core install` / `arduino-cli lib install`)
  — see [Automatic Prerequisite Installation](#automatic-prerequisite-installation) below.
  Pass `--skip-prereqs` to skip this check (offline/CI use, or when you already know your
  environment is set up)
- `-u/--upload` uploads after compiling and opens a serial monitor if a terminal emulator
  is available
- `-e/--erase` toggles `EraseFlash=all`
- `-p/--port` selects the serial device (defaults to `/dev/ttyACM0` for TTGO,
  `/dev/ttyUSB0` for Heltec)
- Creates timestamped backups keyed by `CURRENT_VERSION` before every build
- Accepts `.bkp-*` files to restore previous firmware builds automatically
- Honors `OPTIONS="--build-property ..."` for advanced overrides

Examples:

```bash
# Compile only, TTGO (default target), minimal build (no RTC/IMAP/ChatGPT/GSM)
./scripts/flex-build-upload.sh flex-fsk-tx-v2.ino

# Compile the full-featured build (all four optional subsystems enabled)
./scripts/flex-build-upload.sh --enable-rtc --enable-imap --enable-chatgpt --enable-gsm flex-fsk-tx-v2.ino

# Compile + upload with flash erase, Heltec WiFi LoRa 32 V2
./scripts/flex-build-upload.sh -t heltec -u -e flex-fsk-tx-v2.ino
```

### Automatic Prerequisite Installation

`scripts/flex-build-upload.sh` validates its own build tooling before every compile
(unless `--skip-prereqs` is passed):

1. **`arduino-cli` itself**: only checked, never auto-installed — nothing else can
   bootstrap it. If missing, the script prints the official install command and exits:
   ```bash
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
   ```
2. **ESP32 board core** (`esp32:esp32`): installed automatically via
   `arduino-cli core install esp32:esp32` if not already present.
3. **Arduino libraries**: the always-required set (RadioLib, U8g2, ArduinoJson,
   PubSubClient) plus whichever libraries the requested `--enable-*` flags need (see the
   table in section 3 below) are checked via `arduino-cli lib list "<Name>"` and installed
   via `arduino-cli lib install "<Name>"` if missing.

Every install step prints an explicit `Installing ...` line — nothing happens silently.
This makes the manual "Arduino IDE Setup" steps below a fallback/reference, not a
required prerequisite when building via the script.

> Tip: Run the script from the directory that currently holds your backups or board
> configs — it does not require changing into any specific firmware folder first.

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

Use **Tools → Manage Libraries** to install the following. `scripts/flex-build-upload.sh`
installs these automatically (see [Automatic Prerequisite
Installation](#automatic-prerequisite-installation) above) — this section is for manual
Arduino IDE setups. **Always required** (regardless of any `--enable-*` flag):

| Library | Author | Purpose | Gated by |
|---------|--------|---------|----------|
| **RadioLib** | Jan Gromeš | LoRa/FSK radio control | always |
| **U8g2** | oliver | OLED display control | always |
| **ArduinoJson** | Benoit Blanchon | JSON handling for REST API, Grafana, MQTT, ChatGPT, IMAP, storage | always |
| **PubSubClient** | Nick O'Leary | MQTT client for bidirectional messaging | always |

**Only required when the matching optional subsystem is enabled**:

| Library | Author | Purpose | Gated by |
|---------|--------|---------|----------|
| **RTClib** | Adafruit | DS3231 RTC support | `ENABLE_RTC` (default off) |
| **ReadyMail** | Khoi Hoang | IMAP email client for email-to-pager (search "ReadyMail" or "ESP Mail Client") | `ENABLE_IMAP` (default off) |
| **TinyGsmClient** | Volodymyr Shymanskyy | GSM/GPRS modem support (SIM800L, SIMCOM A7670SA) | `ENABLE_GSM` (default off) |
| **SSLClient** | OPEnSLab-OSU | TLS/SSL over GSM for secure MQTT/IMAP | `ENABLE_GSM` (default off) |

`ENABLE_CHATGPT` needs no extra library — it only uses always-available ESP32-core
classes (`HTTPClient`, `WiFiClientSecure`, `SPIFFS`) plus ArduinoJson.

**Built-in, no installation needed**: Wire (I2C), SPI — both part of the ESP32 core.

**Note**: Board pin definitions are included locally in `include/boards/boards.h`. No
external board-specific pin library is needed.

A default build (no `--enable-*` flags) only needs the four always-required libraries
above. Pass `--enable-rtc`/`--enable-imap`/`--enable-gsm` to pull in RTClib/ReadyMail/
TinyGsmClient+SSLClient respectively.

### 4. Verify Library Installation

**Check installed libraries**: Tools → Manage Libraries → Filter "Installed"

**Expected libraries for a default build (TTGO or Heltec V2, no `--enable-*` flags)**:
- ✅ RadioLib
- ✅ U8g2
- ✅ ArduinoJson
- ✅ PubSubClient
- ✅ Wire, SPI (built-in, no action needed)

**Additionally required per enabled optional subsystem**:
- ✅ RTClib — only if built with `--enable-rtc`
- ✅ ReadyMail (or ESP Mail Client) — only if built with `--enable-imap`
- ✅ TinyGsmClient (TinyGSM) and SSLClient — only if built with `--enable-gsm`

## 📱 Device-Specific Flashing Procedures

### TTGO LoRa32-OLED Flashing

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
   - **Partition Scheme**: Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS) — **required**,
     see [TTGO Build Properties Requirement](#ttgo-build-properties-requirement)
   - **Core Debug Level**: None
   - **Port**: Select your device port (e.g., /dev/ttyACM0, COM3)

#### Flashing

```bash
# Open in Arduino IDE: File → Open → flex-fsk-tx-v2.ino
# Verify all libraries from section 3 are installed
# Set Partition Scheme to "Minimal SPIFFS" (see above)
# Upload: Sketch → Upload

# Or use arduino-cli directly with build properties:
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  flex-fsk-tx-v2.ino

# Or use the flex-build-upload script (recommended, runs from any directory):
./scripts/flex-build-upload.sh -t ttgo flex-fsk-tx-v2.ino
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
- Or use arduino-cli/the build script with build properties (see above)

**Upload speed issues**:
- Try lower upload speed: 115200 or 460800
- Use different USB cable (data cable, not charging only)
- Try different USB port

### Heltec WiFi LoRa 32 V2 Flashing

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
   - **Note**: Heltec V2 uses generic ESP32 board selection (not a Heltec-specific board
     entry)
2. **Configure Settings**:
   - **Upload Speed**: 921600 (or 115200 if upload fails)
   - **CPU Frequency**: 240MHz (WiFi/BT)
   - **Flash Frequency**: 80MHz
   - **Flash Mode**: QIO
   - **Flash Size**: 4MB (32Mb)
   - **Partition Scheme**: Default 4MB with spiffs is normally sufficient on Heltec; use
     Minimal SPIFFS if you hit "Sketch too big" (see TTGO note above — the cause is
     identical)
   - **Core Debug Level**: None
   - **Port**: Select your device port (e.g., /dev/ttyUSB0, COM4)

#### Flashing

```bash
# Open in Arduino IDE: File → Open → flex-fsk-tx-v2.ino
# Add the Heltec board macro so config.h picks the right pin map:
#   Tools → Additional build flags, or compile via arduino-cli/the build script below
# Verify all libraries from section 3 are installed
# Upload: Sketch → Upload

# Or use arduino-cli directly:
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V2 \
  --build-property "compiler.cpp.extra_flags=-DHELTEC_WIFI_LORA32_V2" \
  flex-fsk-tx-v2.ino

# Or use the flex-build-upload script (recommended, sets the macro automatically):
./scripts/flex-build-upload.sh -t heltec flex-fsk-tx-v2.ino
```

#### Upload Troubleshooting (Heltec V2)
**Upload fails or device not detected**:
1. Install CP210x or CH340 USB drivers if needed
2. Try different USB cable
3. Hold **PRG** button during upload process
4. Check Windows Device Manager for driver issues

**Compilation errors**:
- **Missing library errors**: Ensure all required libraries are installed (see section 3)
- **`include/tinyflex/tinyflex.h` missing**: Confirm you're building from an intact clone
  of the repository (see [tinyflex Embedded Library Requirement](#tinyflex-embedded-library-requirement))
- **`include/boards/boards.h` not found**: Same as above — confirm the `include/`
  directory is present next to the `.ino` file
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

This is single-variant firmware — every command below is present on every build (both
TTGO and Heltec). There is no `AT+VERSION?` command; the running version is reported by
`AT+DEVICE?`'s `+DEVICE_FIRMWARE` line, sourced from `version.h`.

```bash
AT+DEVICE?        # Should include +DEVICE_FIRMWARE: <version>, then OK
AT+MSG=1234567    # Should respond: +MSG: READY
AT+WIFI?          # Should respond: +WIFI: DISCONNECTED (or CONNECTED,<ip> / AP_MODE,<ip>)
```

API credentials have no AT command — they're web-interface-only (`/api_config` page).
`AT+DEVICE?`'s `+DEVICE_API` line only reports Enabled/Disabled, not the credentials.

### 4. OLED Display Verification

**Check display shows**:
- **Line 1**: "flex-fsk-tx" banner or device branding
- **Line 2**: Current status (Ready, Transmitting, etc.)
- **Line 3**: Frequency setting
- **Line 4**: Power setting
- **WiFi Status**: Connected/Disconnected + IP address
- **Battery Status**: Percentage and power state (if applicable)

### 5. Feature Testing

**Test PPM correction** (0.02 decimal precision):
```bash
AT+FREQPPM=1.23
# Expected response: OK

AT+FREQPPM?
# Expected: +FREQPPM: 1.23
```

**Test watchdog operations**: device should be stable without unexpected resets; monitor
serial for proper watchdog task registration logs on boot.

**Test Persistent Log System**:
```bash
# Query last 25 log lines (default)
AT+LOGS?
# Expected: timestamped log lines + OK

# Query specific number of lines
AT+LOGS?50
# Expected: last 50 log lines + OK

# Delete log file
AT+RMLOG
# Expected: LOG: File deleted + OK

# Test web log endpoint
curl -s http://DEVICE_IP/logs?lines=20
# Expected: JSON with {"logs":[...]}

# Download full log file
curl -s http://DEVICE_IP/download_logs -o serial.log
```

**Test Network Transport Mode** (only responds to GSM if built with `ENABLE_GSM`):
```bash
# Query current mode
AT+NETWORK?
# Expected: +NETWORK: AUTO

# Lock to WiFi
AT+NETWORK=WIFI
# Expected: OK (display shows WiFi*)

# Return to automatic failover
AT+NETWORK=AUTO
# Expected: OK
```

## 🚨 Troubleshooting

**🔧 Complete Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive firmware and hardware issue resolution.

### Quick Firmware Issues

### Compilation Errors

**"include/tinyflex/tinyflex.h: No such file or directory"**:
```bash
# Solution: confirm the repository's include/ directory is intact next to the .ino file
ls -l include/tinyflex/tinyflex.h

# If you exported the sketch elsewhere, copy the whole include/ directory alongside it:
cp -R include/ /path/to/exported/sketch/
```

**"U8g2lib.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "U8g2" → Install
```

**"ArduinoJson.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "ArduinoJson" → Install
```

**"ReadyMail.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "ReadyMail" or "ESP Mail Client" → Install
```

**"PubSubClient.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager
# Tools → Manage Libraries → Search "PubSubClient" → Install
```

**"RTClib.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager, or don't pass --enable-rtc (RTC support is
# off by default) if you don't have a DS3231 RTC module installed
# Tools → Manage Libraries → Search "RTClib" → Install
```

**"TinyGsmClient.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager, or don't pass --enable-gsm (GSM support is
# off by default) if you don't need GSM/cellular failover
# Tools → Manage Libraries → Search "TinyGSM" → Install
```

**"SSLClient.h: No such file or directory"**:
```bash
# Solution: Install via Library Manager, or don't pass --enable-gsm (see above)
# Tools → Manage Libraries → Search "SSLClient" → Install
```

**"Sketch too big" or "text section exceeds available space"**:
```bash
# Solution 1: Use Minimal SPIFFS partition in Arduino IDE
# Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)

# Solution 2: Use arduino-cli with build properties
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property build.partitions=min_spiffs \
  --build-property upload.maximum_size=1966080 \
  flex-fsk-tx-v2.ino
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

**WiFi features not working**:
```bash
# Check if WiFi AT commands are recognized
AT+WIFI?
# If this returns ERROR, the device may be mid-transmission (only AT, AT+STATUS?, and
# AT+ABORT are accepted during an active transmission) — otherwise check the
# ArduinoJson library installation

# Check ArduinoJson library installation
# Tools → Manage Libraries → Installed → Search "ArduinoJson"
```

### Device-Specific Issues

**TTGO LoRa32-OLED**:
- **Serial port**: Usually `/dev/ttyACM0` on Linux, `COM3+` on Windows
- **Upload mode**: May require BOOT+RESET button sequence
- **Board selection**: "TTGO LoRa32-OLED V1" or "ESP32 Dev Module"
- **Partition scheme**: Must use Minimal SPIFFS (see [TTGO Build Properties Requirement](#ttgo-build-properties-requirement))

**Heltec WiFi LoRa 32 V2**:
- **Serial port**: Usually `/dev/ttyUSB0` on Linux, `COM4+` on Windows
- **Upload mode**: Usually automatic, may need PRG button
- **Board selection**: Must be "ESP32 Dev Module"
- **Radio chipset**: SX1276 (same as TTGO, full 248 character support)

## 📋 Pre-Flash Checklist

Before flashing, verify:

- [ ] **Arduino IDE installed** with ESP32 board support
- [ ] **Device detected** and proper port selected
- [ ] **Required libraries installed** (see section 3 — 4 always-required, plus any tied
      to `--enable-*` flags you're building with; `flex-build-upload.sh` checks/installs
      these automatically)
- [ ] **`include/` directory present** next to `flex-fsk-tx-v2.ino` (tinyflex + boards)
- [ ] **Proper board selected** for your hardware
- [ ] **Board macro/flag set correctly** for Heltec (`-DHELTEC_WIFI_LORA32_V2`), or left
      default for TTGO
- [ ] **Partition scheme set** (Minimal SPIFFS required for TTGO, usually fine as
      default for Heltec)
- [ ] **USB cable supports data** (not just charging)
- [ ] **Antenna connected** to device

## 📚 Related Documentation

For usage after successful firmware installation:
- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface setup and usage
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference
- **[REST_API.md](REST_API.md)**: REST API documentation
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
