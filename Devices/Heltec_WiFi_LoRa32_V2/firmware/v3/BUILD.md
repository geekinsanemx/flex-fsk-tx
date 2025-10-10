# Heltec WiFi LoRa 32 V2 Firmware Build Instructions

## ⚠️ REQUIRED: Copy tinyflex.h Before Compilation

**IMPORTANT:** You must copy `tinyflex.h` from the submodule to this directory before compiling.

```bash
# From repository root
cp include/tinyflex/tinyflex.h Devices/Heltec_WiFi_LoRa32_V2/firmware/v3/
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

**Included with ESP32 Core:**
- WiFiClientSecure
- SPIFFS
- Preferences
- esp_task_wdt
- Wire
- SPI

---

## Compilation

### Arduino IDE

1. Open `Devices/Heltec_WiFi_LoRa32_V2/firmware/v3/heltec_fsk_tx_AT_v3.ino`
2. Select Board: **"Heltec WiFi LoRa 32(V2)"**
3. Verify/Compile
4. Upload

**Standard partition scheme works** - No custom properties needed (unlike TTGO).

### Arduino CLI

```bash
arduino-cli compile \
  --fqbn esp32:esp32:heltec_wifi_lora_32_V2 \
  Devices/Heltec_WiFi_LoRa32_V2/firmware/v3/heltec_fsk_tx_AT_v3.ino
```

### Upload

```bash
arduino-cli upload \
  --fqbn esp32:esp32:heltec_wifi_lora_32_V2 \
  --port /dev/ttyUSB0 \
  Devices/Heltec_WiFi_LoRa32_V2/firmware/v3/heltec_fsk_tx_AT_v3.ino
```

---

## Board Configuration

- **Board**: Heltec WiFi LoRa 32(V2)
- **FQBN**: `esp32:esp32:heltec_wifi_lora_32_V2`
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio**: SX1276 (no message length limitations)
- **Upload Speed**: 921600
- **Flash Frequency**: 80MHz
- **Partition Scheme**: Default (min_spiffs automatically used)
- **Serial Port**: Usually `/dev/ttyUSB0` (Linux), `COM4+` (Windows)

---

## Device Features

### Hardware Specifications
- **Radio Chipset**: SX1276 (433/868/915 MHz)
- **Power Range**: 0 to +20 dBm
- **Display**: 0.96" OLED (128x64, SSD1306)
- **Connectivity**: WiFi 802.11 b/g/n, Bluetooth
- **No Message Limit**: Unlike V3 with SX1262 issues

---

## Firmware Features (v3.6.68)

### Core Functionality
- FLEX paging message transmission (up to 248 characters)
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
cp include/tinyflex/tinyflex.h Devices/Heltec_WiFi_LoRa32_V2/firmware/v3/
```

### Submodule Not Initialized

```bash
git submodule update --init --recursive
ls include/tinyflex/tinyflex.h  # Should exist
```

### Library Not Found

Verify all libraries installed:
```bash
arduino-cli lib list | grep -E "RadioLib|U8g2|ArduinoJson|PubSubClient|ReadyMail"
```

### Display Issues

Heltec V2 uses built-in display management. If display not working:
- Ensure Wire and SPI libraries are available (included with ESP32 core)
- Check OLED connections (should be built-in on official boards)

---

## Updating tinyflex

When updating the tinyflex submodule:

```bash
# Update submodule
cd include/tinyflex
git pull origin master
cd ../..

# Re-copy to firmware directory
cp include/tinyflex/tinyflex.h Devices/Heltec_WiFi_LoRa32_V2/firmware/v3/
```

---

## Heltec V2 vs V3 Comparison

| Feature | V2 (Current) | V3 (Deprecated) |
|---------|--------------|-----------------|
| MCU | ESP32 LX6 | ESP32-S3 LX7 |
| Radio | SX1276 ✅ | SX1262 ⚠️ |
| Message Limit | 248 chars ✅ | ~130 chars ⚠️ |
| Serial Port | /dev/ttyUSB0 | /dev/ttyUSB0 |
| Partition | Standard ✅ | Standard |
| Status | **Supported** | **Deprecated** |

**Why V2?** The V3's SX1262 chipset has chunking transmission issues causing message corruption. V2 with SX1276 has no such limitations.

---

## See Also

- [AT Commands Reference](../../../../docs/AT_COMMANDS.md)
- [REST API Documentation](../../../../docs/REST_API.md)
- [User Guide](../../../../docs/USER_GUIDE.md)
- [Troubleshooting](../../../../docs/TROUBLESHOOTING.md)
