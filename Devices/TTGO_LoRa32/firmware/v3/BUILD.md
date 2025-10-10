# TTGO LoRa32 v3 Firmware Build Instructions

## ⚠️ REQUIRED: Copy tinyflex.h Before Compilation

**IMPORTANT:** You must copy `tinyflex.h` from the submodule to this directory before compiling.

```bash
# From repository root
cp include/tinyflex/tinyflex.h Devices/TTGO_LoRa32/firmware/v3/
```

**Why?** Arduino IDE/CLI requires dependencies in the same directory as the sketch.

---

## Prerequisites

### 1. Initialize tinyflex Submodule

```bash
# Clone with submodules
git clone --recursive https://github.com/geekinsanemx/flex-fsk-tx.git

# OR if already cloned:
git submodule update --init --recursive
```

### 2. Install Required Libraries

**Via Arduino Library Manager:**
- RadioLib (latest)
- U8g2 (latest)
- ArduinoJson v6+ (latest)
- PubSubClient (latest)
- ReadyMail (latest)

**Manual Installation:**
```bash
# RadioBoards library (required for TTGO)
git clone https://github.com/radiolib-org/RadioBoards ~/Arduino/libraries/RadioBoards
```

**Included with ESP32 Core:**
- WiFiClientSecure
- SPIFFS
- Preferences
- esp_task_wdt

---

## Compilation

### ⚠️ CRITICAL: Custom Partition Scheme Required

TTGO v3 firmware (~400KB) **requires custom partition scheme** due to size.

**Without these properties, compilation will fail with "Sketch too big" error.**

### Arduino IDE

1. Open `Devices/TTGO_LoRa32/firmware/v3/ttgo_fsk_tx_AT_v3.ino`
2. Select Board: **"TTGO LoRa32-OLED"** or **"ESP32 Dev Module"**
3. **IMPORTANT**: You must use Arduino CLI or custom build script - Arduino IDE GUI doesn't support custom partition properties
4. See Arduino CLI method below

### Arduino CLI (Recommended)

```bash
arduino-cli compile \
  --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --build-property "build.partitions=min_spiffs" \
  --build-property "upload.maximum_size=1966080" \
  Devices/TTGO_LoRa32/firmware/v3/ttgo_fsk_tx_AT_v3.ino
```

### Custom Build Script

If you have `ttgo-build-upload.sh`:

```bash
OPTIONS="--build-properties build.partitions=min_spiffs,upload.maximum_size=1966080" \
  ttgo-build-upload.sh Devices/TTGO_LoRa32/firmware/v3/ttgo_fsk_tx_AT_v3.ino
```

### Upload

```bash
arduino-cli upload \
  --fqbn esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new \
  --port /dev/ttyACM0 \
  Devices/TTGO_LoRa32/firmware/v3/ttgo_fsk_tx_AT_v3.ino
```

---

## Board Configuration

- **Board**: TTGO LoRa32-OLED
- **FQBN**: `esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new`
- **Upload Speed**: 921600
- **Flash Frequency**: 80MHz
- **Partition Scheme**: `min_spiffs` (via build property)
- **Maximum Size**: 1966080 bytes (via build property)
- **Serial Port**: Usually `/dev/ttyACM0` (Linux), `COM3+` (Windows)

---

## Firmware Features (v3.6.68)

### Core Functionality
- FLEX paging message transmission
- Message queue (25 messages)
- EMR (Emergency Message Resynchronization)
- Battery monitoring with alerts

### Network Integration
- WiFi Station + AP mode
- Web interface (port 80)
- REST API (port 16180)
- mDNS support

### Advanced Features
- **IMAP Email-to-Pager**: Up to 5 accounts, auto-fetch and transmit
- **MQTT Bidirectional**: QoS 1, persistent sessions, TLS/SSL
- **ChatGPT Scheduled Prompts**: Up to 10 prompts with scheduling
- **Grafana Webhook**: Alert integration (`/api/v1/alerts`)
- **Remote Syslog**: RFC 3164 compliant, UDP/TCP

### System Features
- Watchdog timer protection
- Chunked HTTP responses (87% memory reduction)
- SPIFFS storage (settings, IMAP, MQTT certs)
- Security hardening (XSS protection, timing attack mitigation)
- 2 UI themes (Minimal White, Carbon Black)

---

## Troubleshooting

### "tinyflex.h: No such file or directory"

**Solution:** Copy tinyflex.h to firmware directory
```bash
cp include/tinyflex/tinyflex.h Devices/TTGO_LoRa32/firmware/v3/
```

### Submodule Not Initialized

```bash
git submodule update --init --recursive
ls include/tinyflex/tinyflex.h  # Should exist
```

### "Sketch too big" Error

**You forgot the custom partition properties!** Use Arduino CLI with:
```bash
--build-property "build.partitions=min_spiffs" \
--build-property "upload.maximum_size=1966080"
```

### Library Not Found

Verify all libraries installed:
```bash
arduino-cli lib list | grep -E "RadioLib|U8g2|ArduinoJson|PubSubClient|ReadyMail"
```

Check RadioBoards manual installation:
```bash
ls ~/Arduino/libraries/RadioBoards
```

---

## Updating tinyflex

When updating the tinyflex submodule:

```bash
# Update submodule
cd include/tinyflex
git pull origin master
cd ../..

# Re-copy to firmware directory
cp include/tinyflex/tinyflex.h Devices/TTGO_LoRa32/firmware/v3/
```

---

## See Also

- [AT Commands Reference](../../../../docs/AT_COMMANDS.md)
- [REST API Documentation](../../../../docs/REST_API.md)
- [User Guide](../../../../docs/USER_GUIDE.md)
- [Troubleshooting](../../../../docs/TROUBLESHOOTING.md)
