# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**flex-fsk-tx** is a FLEX pager message transmission system for ESP32 LoRa32 devices. It is a
subsystem-per-file restructuring of the original [flex-fsk-tx](https://github.com/geekinsanemx/flex-fsk-tx)
`v3.8_GSM` firmware (a single 13,911-line `.ino` sketch), split into one `.cpp`/`.h` pair per concern.
It is intended to eventually replace that repository entirely, not sit alongside it as a partial copy.
The port is mechanical: no behavior, feature, or logic changes relative to `v3.8.67` — every function,
global, and struct was relocated as-is, with cross-module globals promoted to `extern` only where
genuinely shared across translation units.

The project has three parts:

1. **ESP32 Firmware** (repo root, flat `.cpp`/`.h` files + `flex-fsk-tx.ino`) — WiFi + GSM/cellular
   dual-transport networking with automatic failover, a web configuration/control interface, a REST API,
   MQTT, IMAP-triggered paging, scheduled ChatGPT prompts, a Grafana webhook receiver, and FLEX protocol
   transmission over SX1276 hardware.
2. **Host Application** (`host/flex-fsk-tx.cpp`) — optional PC-side C++ CLI that sends FLEX messages and
   configures the device purely over its serial AT command interface (no network dependency).
3. **tinyflex Library** (`include/tinyflex/tinyflex.h`) — embedded single-header FLEX protocol library,
   shared by both the firmware and the host application.

## Supported Hardware

- **TTGO LoRa32-OLED** (ESP32 + SX1276) — fully supported, 248-character messages
- **Heltec WiFi LoRa 32 V2** (ESP32 + SX1276) — fully supported, 248-character messages

Both boards share the same firmware; board selection is a compile-time `#define` (`TTGO_LORA32_V21` or
`HELTEC_WIFI_LORA32_V2` in `config.h`, defaulting to TTGO if neither is set), with pin differences
resolved via `include/boards/boards.h`. Heltec WiFi LoRa 32 V3 (SX1262 chipset) is not supported.

## Build System

### Host Application (C++)

```bash
cd host
make              # Build the host application
make debug        # Build with debug symbols
sudo make install # Install to system
make clean        # Clean build artifacts
make check-deps   # Verify tinyflex dependency is present
```

**Prerequisites**: g++ (C++11), and `include/tinyflex/tinyflex.h` (already vendored in this repo — no
submodule init needed here).

### ESP32 Firmware

```bash
# Compile-only (no upload)
./scripts/flex-build-upload.sh -t ttgo flex-fsk-tx.ino
./scripts/flex-build-upload.sh -t heltec flex-fsk-tx.ino

# Compile and upload (-u), optionally erasing flash first (-e)
./scripts/flex-build-upload.sh -t ttgo -p /dev/ttyACM0 -u flex-fsk-tx.ino
```

The script backs up the sketch to `bkp/` (named from `version.h`'s `FIRMWARE_VERSION`) before every
build, then drives `arduino-cli compile` with the correct FQBN/build properties per board. See
[docs/FIRMWARE.md](docs/FIRMWARE.md) for Arduino IDE setup and library dependencies if not using the
script.

### ESP32 Firmware (PlatformIO, alternative to arduino-cli)

```bash
pio run -e ttgo-wifi                # TTGO, WiFi only (no RTC/IMAP/ChatGPT/GSM)
pio run -e ttgo-wifi-all            # TTGO, WiFi + RTC/IMAP/ChatGPT (no GSM)
pio run -e ttgo-gsm                 # TTGO, GSM only (no RTC/IMAP/ChatGPT)
pio run -e ttgo-gsm-rtc             # TTGO, GSM + RTC (IMAP/ChatGPT are auto-suspended under GSM anyway)
pio run -e ttgo-full                # TTGO, everything (RTC/IMAP/ChatGPT/GSM)
# heltec-wifi / heltec-wifi-all / heltec-gsm / heltec-gsm-rtc / heltec-full mirror the above

pio run -e ttgo-wifi -t upload -p /dev/ttyACM0   # compile and upload
```

`ENABLE_IMAP`/`ENABLE_CHATGPT` are intentionally omitted from the `-gsm`/`-gsm-rtc` profiles: both
services are already auto-suspended at runtime whenever GSM is the active transport (see
`src/network/network.cpp`'s transport arbitration), so building them in doesn't add capability for
that profile — `-full` remains available for anyone who wants every flag enabled regardless.

`platformio.ini` (repo root) points PlatformIO's `src_dir` at the repo root and uses
`build_src_filter` to compile exactly `flex-fsk-tx.ino` plus `src/**` — the identical source
tree the arduino-cli path builds, with `host/`, `bkp/`, `scripts/`, `docs/`, and
`include/tinyflex/demos/` excluded. No source file is PlatformIO-specific; this is purely an
additive second build system, and the arduino-cli/`scripts/flex-build-upload.sh` path is
unaffected and remains the primary/documented one.

`platform` is pinned to the [pioarduino](https://github.com/pioarduino/platform-espressif32) fork
of `espressif32`, not the official PlatformIO Registry `espressif32` platform — the official one
tops out at arduino-esp32 core 2.0.17 (ESP-IDF 4.4), which lacks the ESP-IDF 5.x watchdog API
(`esp_task_wdt_config_t`, used in `src/core/hardware.cpp`) that this codebase requires and that
arduino-cli's `esp32:esp32` core (3.x) already provides. pioarduino is the community-standard
source for arduino-esp32 3.x under PlatformIO.

Flash usage under PlatformIO's `-full` environments (~1.91 MB / 97% on TTGO) runs noticeably
higher than the arduino-cli build of the same feature set (~1.69 MB / 85%) — both compile and
link cleanly, but the difference (traced to newer bundled library/toolchain versions pulled from
the PlatformIO registry, e.g. RadioLib 7.7.1 vs whatever arduino-cli's Library Manager last
installed) leaves less flash headroom on the TTGO's `min_spiffs` partition table. Worth
rechecking if a future library bump pushes this closer to the 1,966,080-byte ceiling.

## Architecture

### File Layout

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

src/web/web_server.cpp/h              HTTP server core, HTML header/footer, route registration
src/web/web_handlers_settings.cpp     configuration pages (FLEX, MQTT, IMAP, API, GSM) + save handlers
src/web/web_handlers_device.cpp       status, logs, backup/restore, certificate upload, factory reset
src/web/web_handlers_api.cpp          REST API (message send, WiFi scan/add/delete)
src/web/web_handlers_chatgpt.cpp       ChatGPT scheduler page + prompt CRUD

host/                          PC-side CLI companion (flex-fsk-tx.cpp, Makefile) — see host/README.md
include/boards/                board-specific pin definitions (TTGO / Heltec)
include/gsm_trust_anchors/     GSM TLS root CA bundle
include/tinyflex/              embedded FLEX encoding library
scripts/flex-build-upload.sh   arduino-cli build/upload automation
docs/                          AT_COMMANDS.md, REST_API.md, USER_GUIDE.md, FIRMWARE.md, TROUBLESHOOTING.md, QUICKSTART.md
```

Local `#include`s inside `src/<group>/` files are written relative to that file's own directory,
not to `src/` or the repo root — e.g. `src/network/ntp_time.h` includes `config.h` (in a
different group) as `#include "../core/config.h"`, and `../../include/boards/boards.h` for the
repo-root `include/` tree. This project's toolchain (arduino-cli + esp32:esp32 platform) does
**not** add `src/` itself to the compiler's include search path, despite what Arduino's sketch
specification docs suggest — only true quote-include directory-relative resolution works here,
verified empirically via compile failures when bare `"core/config.h"`-style includes were tried
from within other subgroups. The `.ino` itself, which lives at the repo root, includes files as
`#include "src/version.h"` etc. — also directory-relative, just from its own (repo-root)
location.

### Compile-time optional subsystems (config.h)

- `ENABLE_GSM` — GSM/cellular failover support (comment out for WiFi-only builds; this is this
  project's architectural simplification versus the old repo's separate `v3`/`v4` firmware split —
  one firmware, GSM toggled by a single `#define`)
- `ENABLE_IMAP` — IMAP email-to-page polling
- `ENABLE_DEBUG` — verbose debug output
- `ENABLE_RTC` — DS3231 RTC support

### Radio defaults (config.h)

`TX_FREQ_DEFAULT` is `931.9375` MHz, unified for **both** TTGO and Heltec — a deliberate
simplification. (The old repo used different per-board defaults: TTGO 915.0 MHz, Heltec 929.6625 MHz.)
`TX_POWER_DEFAULT` is `2` dBm. Valid range differs by interface: `AT+POWER` accepts `-9` to `20` dBm
(`at_commands.cpp`), while `AT+FLEX=POWER,<v>` and the web/REST API clamp to `0`–`20` dBm
(`at_commands.cpp`'s `AT+FLEX` handler, `web_handlers_api.cpp`).

### AT Command Protocol

Parsed in `at_commands.cpp` (`at_parse_command()`, hand-rolled `strncmp`/`strchr` dispatch — not a
table-driven parser). This is the **complete** current command set; do not assume commands from the
old repo's docs exist here (`AT+MAILDROP`, `AT+WIFIENABLE`, `AT+WIFICONFIG`, `AT+BANNER`, `AT+APIPORT`,
`AT+APIUSER`, `AT+APIPASS`, `AT+BATTERY`, `AT+SAVE`, `AT+GRAFANA`, `AT+SETDEFAULT`, `AT+GETDEFAULT` are
all gone):

- `AT` — bare liveness check, returns `OK`
- `AT+FREQ=<MHz>` / `AT+FREQ?` — set/query transmit frequency
- `AT+FREQPPM=<ppm>` — frequency correction (PPM)
- `AT+POWER=<dBm>` / `AT+POWER?` — set/query transmit power
- `AT+SEND=<len>` — binary FLEX frame transmission (host encodes locally via tinyflex, then streams
  `<len>` raw bytes)
- `AT+MSG=<capcode>,<text>` — device-side FLEX encoding + transmission (remote encoding mode)
- `AT+STATUS?` — device status
- `AT+ABORT` — abort in-progress transmission
- `AT+RESET` — restart device
- `AT+NETWORK=<AUTO|WIFI|GSM|AP>` / `AT+NETWORK?` — lock/query network transport mode (disables
  automatic failover when locked; resets to AUTO on reboot)
- `AT+WIFI=<ssid>,<password>` / `AT+WIFI?` — configure/query WiFi
- `AT+DEVICE?` — multi-line status dump (`+DEVICE_FIRMWARE`, `+DEVICE_BATTERY`, `+DEVICE_WIFI`,
  `+DEVICE_MQTT`, `+DEVICE_IMAP` per account, `+DEVICE_API`, `+DEVICE_GRAFANA`, `+DEVICE_MEMORY`,
  `+DEVICE_FLEX_CAPCODE/FREQUENCY/POWER`)
- `AT+FLEX=CAPCODE,<v>` / `FREQUENCY,<v>` / `POWER,<v>`, and `AT+FLEX?` — set/query default FLEX
  settings (capcode/frequency/power used when a value isn't explicitly given); every set auto-saves via
  `save_runtime_settings()` — there is no separate save step
- `AT+FACTORYRESET` — reset all settings to factory defaults and restart
- `AT+LOGS?N` — query last N lines of the persistent SPIFFS log (default 25)
- `AT+RMLOG` — delete the persistent log file

**Response format**: most queries return one or more `+<NAME>: <value>\r\n` lines followed by `OK\r\n`
(errors return `ERROR\r\n`). `AT+FLEX?` and `AT+DEVICE?` are multi-line — any client-side reader must
accumulate all `+`-prefixed lines before the terminating `OK`/`ERROR`, not just the last one seen.

**Mail drop**: the FLEX mail-drop bit is set entirely by the *local* encoder (tinyflex, used by
`AT+SEND`) — it is a frame bit, not an AT command. There is no remote-encoding (`AT+MSG`) equivalent in
this firmware; `host/flex-fsk-tx.cpp`'s `-m/--maildrop` flag only has effect in local-encoding mode.

## Common Development Tasks

### Testing the host application

```bash
# Local encoding (default) — host encodes via tinyflex, sends binary with AT+SEND
echo "1234567:Test Message" | ./host/bin/flex-fsk-tx -d /dev/ttyUSB0 -

# Remote encoding — device encodes via AT+MSG
echo "1234567:Test Message" | ./host/bin/flex-fsk-tx -d /dev/ttyACM0 -r -

# Interactive configuration wizard (AT+FLEX defaults + AT+NETWORK/AT+WIFI)
./host/bin/flex-fsk-tx --config /dev/ttyUSB0

# Factory reset
./host/bin/flex-fsk-tx --factoryreset /dev/ttyUSB0
```

### Testing AT commands directly

```bash
screen /dev/ttyUSB0 115200   # Heltec
screen /dev/ttyACM0 115200   # TTGO

# Example session
AT
AT+FREQ=931.9375
AT+POWER=10
AT+MSG=1234567
# device replies "+MSG: READY", then send the text as a separate line:
Hello from AT+MSG
AT+FLEX?
AT+NETWORK?
AT+DEVICE?
```

### Testing the web interface / REST API

```bash
curl -s http://DEVICE_IP/ | grep -o "flex-fsk-tx"
curl -s http://DEVICE_IP/status | grep -o "System Information"

# REST API (HTTP Basic Auth, configurable via the web /api_config page)
curl -X POST http://DEVICE_IP/api \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":931.9375,"power":10,"message":"API Test"}'

# Grafana alert webhook
curl -X POST http://DEVICE_IP/api/v1/alerts -H "Content-Type: application/json" -d '{...}'

# Persistent log endpoint
curl -s "http://DEVICE_IP/logs?lines=50"
curl -s http://DEVICE_IP/download_logs -o serial.log
```

Full endpoint list is registered in `web_server.cpp`'s `web_server_init()`; see
[docs/REST_API.md](docs/REST_API.md) and [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for details.

### Device detection

- **Heltec WiFi LoRa 32 V2**: usually `/dev/ttyUSB0` (Linux), `COM4+` (Windows)
- **TTGO LoRa32-OLED**: usually `/dev/ttyACM0` (Linux), `COM3+` (Windows)
- Check with `ls /dev/tty*` or `dmesg | tail` (Linux)

## AP Mode

When neither WiFi nor GSM connects, the device raises a WiFi AP. SSID and password are MAC-derived
(`generate_ap_ssid()`/`generate_ap_password()` in `wifi.cpp`): SSID is `FLEX_XXXX` (4 hex chars from the
MAC), password is an 8-hex-char string from 4 MAC bytes — both unified across TTGO/Heltec (no more
per-board `TTGO_FLEX_XXXX`/`HELTEC_FLEX_XXXX` prefixes). Access configuration at `http://192.168.4.1`.

## Debugging

**Complete troubleshooting**: see [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) for firmware,
hardware, and Arduino IDE environment issues.

**Quick checks**:
- Device not responding: check serial port/cable, confirm `SERIAL_BAUD` (115200) matches
- `AT+MSG` behaving oddly: mail drop is silently ignored in remote-encoding mode (see above) — this is
  expected, not a bug
- Permission denied opening `/dev/ttyUSB*`/`/dev/ttyACM*`: add user to the `dialout` group
- Compilation errors: verify RadioLib, U8g2 (TTGO)/Heltec ESP32 Dev-Boards (Heltec), ArduinoJson,
  TinyGSM + SSLClient (GSM), PubSubClient, RTClib are installed via Library Manager

## Version Information

`version.h`'s `FIRMWARE_VERSION` is the single source of truth for the running firmware version — every
module that reports a version includes this header rather than defining its own copy. See
[CHANGELOG.md](CHANGELOG.md) for the full per-patch history carried over from the original monolithic
sketch, plus notes on this restructuring itself.
