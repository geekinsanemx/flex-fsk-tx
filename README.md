# flex-fsk-tx

**FLEX Paging Message Transmitter firmware — subsystem-per-file restructuring of `flex-fsk-tx-v3.8_GSM`**

This project is a structural port of the [flex-fsk-tx](https://github.com/geekinsanemx/flex-fsk-tx) `v3.8_GSM` firmware — a single 13,911-line Arduino `.ino` sketch — into one `.cpp`/`.h` pair per subsystem. It is a mechanical decomposition only: no behavior, feature, or logic changes were made. Every function, global variable, and struct was relocated as-is, with cross-module globals promoted to `extern` only where they are genuinely read/written across translation units; everything else remains file-local `static`.

The firmware itself is unchanged from `v3.8.67`: WiFi + GSM/cellular dual-transport networking with automatic failover, a web configuration/control interface, a REST API, MQTT, IMAP-triggered paging, scheduled ChatGPT prompts, a Grafana webhook receiver, and FLEX protocol transmission over SX1276 hardware. Unlike the original repository, which distributes several firmware generations (`v1`/`v2`/`v3.6`/`v3.8`) side by side as separate `.ino` sketches, this repository is a **single, current firmware variant** with every feature always present — GSM support is a compile-time toggle (`ENABLE_GSM` in `config.h`), not a separate build.

---

## Why this exists

The original monolithic `.ino` mixed all subsystems in one file, making it hard to navigate, review, or extend safely. This project reorganizes the same code along the structural conventions used by the [FlexDevice](https://github.com/geekinsanemx) firmware line: one `.cpp`/`.h` pair per concern, banner-commented header sections, `#define`-only `config.h`, structs defined in the module that owns them, `extern` declarations for cross-module state, and a thin `.ino` that only wires modules together in `setup()`/`loop()`.

## Project vision

**flex-fsk-tx** turns ESP32 LoRa32 development boards into FLEX paging message transmitters, usable by ham radio operators, system integrators, hobbyists, and business users alike:

- **Hardware flexibility** — one firmware, two supported ESP32 LoRa32 platforms
- **Interface diversity** — serial AT commands (with an optional PC-side CLI), a web interface, and a REST API, all available simultaneously on every build
- **Network resilience** — automatic WiFi → GSM → AP failover with reconnection attempts in the background
- **Community-driven** — open source, GPL-3.0, built on prior open FLEX/ESP32 work (see [Acknowledgments](#acknowledgments))

## Interface ecosystem

All three interfaces are available on every build — there is no firmware-tier gating.

### Serial AT commands + optional host CLI
Hayes-style command set for configuration, transmission, and status queries — see
[docs/AT_COMMANDS.md](docs/AT_COMMANDS.md). The optional [`host/`](host/README.md) C++ CLI
application wraps this interface: local (host-side, tinyflex) or remote (device-side) message
encoding, an interactive configuration wizard, and factory reset — all purely over serial, no
network dependency.

### Web interface
Browser-based message transmission, live status dashboard, and full configuration portal (WiFi,
FLEX defaults, MQTT, IMAP, ChatGPT, Grafana, GSM) on port 80. See
[docs/USER_GUIDE.md](docs/USER_GUIDE.md).

### REST API
JSON API with HTTP Basic Authentication, a message queue (up to 25 concurrent requests processed
sequentially), and a Grafana-alert webhook endpoint. See [docs/REST_API.md](docs/REST_API.md).

## Supported hardware

| | TTGO LoRa32-OLED (LilyGO) | Heltec WiFi LoRa 32 V2 |
|---|---|---|
| MCU | ESP32, 240 MHz dual-core | ESP32, 240 MHz dual-core |
| Radio | Semtech SX1276 | Semtech SX1276 |
| Display | 0.96" OLED, 128x64 (SSD1306) | 0.96" OLED, 128x64 (SSD1306) |
| Serial (Linux) | typically `/dev/ttyACM0` | typically `/dev/ttyUSB0` |
| TX power | -9 to 20 dBm | -9 to 20 dBm |
| Message length | up to 248 characters | up to 248 characters |

Both boards share this same firmware; board selection is a compile-time `#define`
(`TTGO_LORA32_V21` or `HELTEC_WIFI_LORA32_V2` in `config.h`, defaulting to TTGO), with pin
differences resolved via `include/boards/boards.h`.

### Hardware acquisition

- **TTGO LoRa32-OLED**: AliExpress, Banggood, Amazon (~$15-25 USD; confirm the OLED display is
  included)
- **Heltec WiFi LoRa 32 V2**: [Heltec Automation](https://heltec.org/) official store, or
  Digi-Key/Mouser/Arrow/AliExpress (~$15-25 USD; confirm the **V2** variant — ESP32 + SX1276, not
  the V3 with ESP32-S3)

## File layout

```
flex-fsk-tx.ino             orchestration only — setup()/loop() calling each module's _init()

src/version.h                  FIRMWARE_VERSION + build metadata + full changelog

src/core/config.h              #define-only: pins, timeouts, buffer sizes, protocol constants
src/core/storage.cpp/h         CoreConfig, DeviceSettings, Preferences, SPIFFS certificate I/O
src/core/logging.cpp/h         log ring buffer, syslog forwarding
src/core/hardware.cpp/h        battery, heartbeat LED, watchdog, boot failure tracking, factory reset button
src/core/display.cpp/h         U8G2 OLED display
src/core/utils.cpp/h           base64, HTML/JSON escaping, CRC32, IP string helpers

src/protocol/flex_protocol.cpp/h   FLEX encoding, EMR, frequency correction, capcode validation
src/protocol/transmission.cpp/h    message queue, Core 0 transmission task, SX1276 radio driver
src/protocol/at_commands.cpp/h     AT command parser (serial control interface)

src/network/wifi.cpp/h         WiFi scan/connect, AP mode, stored network list
src/network/gsm.cpp/h          GSM/cellular modem control (SIM800L / A7670SA via TinyGSM)
src/network/network.cpp/h      WiFi <-> GSM <-> AP failover arbitration
src/network/ntp_time.cpp/h     NTP sync, DS3231 RTC

src/services/mqtt.cpp/h        AWS IoT style MQTT client, activity log
src/services/imap.cpp/h        IMAP polling, scheduled mailbox checks
src/services/chatgpt.cpp/h     scheduled ChatGPT prompt execution
src/services/grafana.cpp/h     Grafana webhook receiver

src/web/web_server.cpp/h              HTTP server core, HTML header/footer, route registration (web_server_init)
src/web/web_handlers_settings.cpp     configuration pages (FLEX, MQTT, IMAP, API, GSM) + save handlers
src/web/web_handlers_device.cpp       status, logs, backup/restore, certificate upload, factory reset
src/web/web_handlers_api.cpp          REST API (message send, WiFi scan/add/delete)
src/web/web_handlers_chatgpt.cpp       ChatGPT scheduler page + prompt CRUD

host/                          optional PC-side CLI companion (see host/README.md)
include/boards/                board-specific pin definitions (TTGO / Heltec)
include/gsm_trust_anchors/     GSM TLS root CA bundle
include/tinyflex/              embedded FLEX encoding library
scripts/flex-build-upload.sh   arduino-cli build/upload automation
docs/                          AT_COMMANDS.md, REST_API.md, USER_GUIDE.md, FIRMWARE.md, TROUBLESHOOTING.md, QUICKSTART.md
```

## Building

### Firmware

```bash
# Compile-only, using the provided build script (recommended — auto-backs up the sketch first)
./scripts/flex-build-upload.sh -t ttgo flex-fsk-tx.ino
./scripts/flex-build-upload.sh -t heltec flex-fsk-tx.ino

# Compile + upload, custom port, optionally erasing flash first
./scripts/flex-build-upload.sh -t ttgo -p /dev/ttyACM0 -u -e flex-fsk-tx.ino
```

Or drive `arduino-cli` directly:

```bash
# TTGO LoRa32 V2.1
arduino-cli compile --fqbn "esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new,FlashFreq=80,UploadSpeed=921600,DebugLevel=none,EraseFlash=none" \
  --build-property "build.partitions=min_spiffs" \
  --build-property "upload.maximum_size=1966080" \
  flex-fsk-tx.ino

# Heltec WiFi LoRa 32 V2
arduino-cli compile --fqbn "esp32:esp32:heltec_wifi_lora_32_V2:CPUFreq=240,UploadSpeed=921600,DebugLevel=none,LORAWAN_REGION=0,LoRaWanDebugLevel=0,LORAWAN_DEVEUI=0,LORAWAN_PREAMBLE_LENGTH=0,EraseFlash=none" \
  flex-fsk-tx.ino
```

See [docs/FIRMWARE.md](docs/FIRMWARE.md) for Arduino IDE setup and library dependencies.

Or, as an alternative, build with PlatformIO — it reads the exact same `src/` tree and
`flex-fsk-tx.ino`, purely additive to the arduino-cli path above:

```bash
pio run -e ttgo-wifi                # TTGO, WiFi only (no RTC/IMAP/ChatGPT/GSM)
pio run -e ttgo-wifi-all            # TTGO, WiFi + RTC/IMAP/ChatGPT (no GSM)
pio run -e ttgo-gsm                 # TTGO, GSM only (no RTC/IMAP/ChatGPT)
pio run -e ttgo-gsm-rtc             # TTGO, GSM + RTC (IMAP/ChatGPT auto-suspend under GSM anyway)
pio run -e ttgo-full                # TTGO, everything (RTC/IMAP/ChatGPT/GSM)
# heltec-wifi / heltec-wifi-all / heltec-gsm / heltec-gsm-rtc / heltec-full mirror the above

pio run -e ttgo-wifi -t upload -p /dev/ttyACM0   # compile and upload
```

`platformio.ini` pins the `espressif32` platform to the
[pioarduino](https://github.com/pioarduino/platform-espressif32) fork rather than the official
PlatformIO Registry one, since the official platform only bundles arduino-esp32 core 2.0.17
(ESP-IDF 4.4) and this codebase needs the ESP-IDF 5.x watchdog API that arduino-cli's
`esp32:esp32` 3.x core already provides.

### Optional host CLI

```bash
cd host
make              # Build
sudo make install # Install system-wide (optional)
```

See [host/README.md](host/README.md) for usage — sending messages and configuring the device
purely over serial/AT commands, no network dependency.

## Quick start

```bash
# 1. Flash the firmware (see Building, above)

# 2. Send a message — pick whichever interface fits:

# Host CLI (optional, serial only)
./host/bin/flex-fsk-tx -d /dev/ttyUSB0 1234567 "Hello World"

# Web interface
# http://DEVICE_IP/ -> fill form -> Send Message

# REST API
curl -X POST http://DEVICE_IP/api -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"message":"Hello World"}'
```

New to the project? Start with [docs/QUICKSTART.md](docs/QUICKSTART.md).

## Technical specifications

- **Protocol**: FLEX (Forward Link EXchange) paging standard, FSK modulation
- **Data rate**: 1.6 kbps, 5 kHz frequency deviation, 10.4 kHz receive bandwidth
- **Frequency range**: 400-1000 MHz (hardware dependent); unified default `931.9375` MHz
  (`TX_FREQ_DEFAULT` in `config.h`)
- **TX power**: -9 to 20 dBm
- **Capcode range**: 1 to 4,297,068,542
- **Message length**: up to 248 characters (auto-truncated if longer)
- **Serial**: 115200 baud, 8N1
- **WiFi**: 802.11 b/g/n (2.4 GHz), WPA2-PSK, DHCP or static IP
- **AP mode SSID/password**: MAC-derived, `FLEX_XXXX` (4 hex chars) SSID with an 8-hex-char
  password, unified across both boards — see `wifi.cpp`
- **Web/API**: HTTP on port 80, HTTP Basic Auth for the REST API, message queue up to 25 requests

## Use case scenarios

- **Amateur radio**: frequency/power experimentation, direct AT command access, emergency
  communications
- **Business/professional**: staff notification systems, priority broadcasts (mail drop), REST
  API integration with existing systems, remote monitoring
- **Home automation/IoT**: Home Assistant/OpenHAB integration, security alerts, sensor-triggered
  notifications, family paging
- **Education/research**: FLEX protocol learning, ESP32 + radio integration projects
- **Legacy modernization**: self-hosted replacement for aging pager infrastructure

## Documentation

- [docs/QUICKSTART.md](docs/QUICKSTART.md) — unboxing to first transmission
- [docs/FIRMWARE.md](docs/FIRMWARE.md) — Arduino IDE setup, flashing procedures
- [docs/USER_GUIDE.md](docs/USER_GUIDE.md) — web interface manual
- [docs/AT_COMMANDS.md](docs/AT_COMMANDS.md) — serial AT command reference
- [docs/REST_API.md](docs/REST_API.md) — REST API reference
- [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) — common issues
- [host/README.md](host/README.md) — optional PC-side CLI companion
- [CLAUDE.md](CLAUDE.md) — architecture and development notes

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for the full firmware version history (carried over unchanged from `version.h`) plus notes on this project's restructuring.

## Community and support

1. Check the documentation above first
2. Review [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) for common issues
3. Report problems via GitHub issues, using the templates in TROUBLESHOOTING.md

## Acknowledgments

This firmware builds on the original [flex-fsk-tx](https://github.com/geekinsanemx/flex-fsk-tx) project, which itself builds on:

- **[Davidson Francis (Theldus)](https://github.com/Theldus)** — original [tinyflex](https://github.com/Theldus/tinyflex) library
- **[Rodrigo Laneth](https://github.com/rlaneth)** — original [ttgo-fsk-tx](https://github.com/rlaneth/ttgo-fsk-tx/) ESP32 firmware
- **Arduino/ESP32 community, RadioLib project** — development framework and radio control library
- **Heltec Automation & LilyGO** — hardware platforms

## License

GNU General Public License v3.0 (GPL-3.0). See [LICENSE](LICENSE).
