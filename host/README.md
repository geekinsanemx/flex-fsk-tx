# flex-fsk-tx Host Application (Optional)

## Overview

This C++ application provides **optional** serial communication with ESP32 devices using AT commands.

**Not required for normal operation.** The firmware includes:
- Web interface (port 80) — primary user interface
- REST API (port 80, `/api` endpoint) — programmatic control
- MQTT integration — IoT messaging
- IMAP integration — email-to-pager

## When to Use This Tool

**Use the host application if:**
- Testing AT commands during firmware development
- Operating the device without WiFi (serial-only mode)
- Batch processing messages via stdin
- Integrating with legacy systems requiring serial control
- Configuring the device (default FLEX settings, network mode, WiFi credentials, factory reset)
  purely over serial, without joining its web interface

**Use the web interface instead if:**
- Normal message transmission
- IMAP/MQTT/ChatGPT/Grafana setup
- System monitoring

## Build & Install

```bash
make              # Build
sudo make install # Install system-wide (optional)
make clean        # Clean build artifacts
make debug        # Debug build
make check-deps   # Verify tinyflex dependency is present
```

## Prerequisites

- C++ compiler (g++, C++11)
- `include/tinyflex/tinyflex.h` (already vendored at `../include/tinyflex/` in this repo)
- Serial port access permissions

## Usage

```
flex-fsk-tx [options] <capcode> <message>
flex-fsk-tx [options] [-l] [-m] [-r] - (from stdin)
flex-fsk-tx --config|-c <device> (interactive configuration)
flex-fsk-tx --factoryreset <device> (factory reset device)
flex-fsk-tx --help (show this help)

Options:
   -d, --device <dev>    Serial device (default: /dev/ttyUSB0)
   -b, --baudrate <rate> Baudrate (default: 115200)
   -f, --frequency <MHz> Frequency in MHz (default: 916.0)
   -p, --power <dBm>     TX power (default: 2, -9 to 20 dBm)
   -l, --loop            Loop mode: stays open receiving new lines until EOF
   -m, --maildrop        Mail Drop flag (local encoding only; ignored with -r)
   -r, --remote          Remote encoding: device encodes via AT+MSG instead of
                         local tinyflex encoding + AT+SEND
   -c, --config <device> Interactive configuration wizard
       --factoryreset <device>  Factory reset the device
   -h, --help            Show this help message and exit
```

### Single message

```bash
./bin/flex-fsk-tx -d /dev/ttyACM0 1234567 "Test message"
```

### Stdin mode

```bash
echo "1234567:Hello World" | ./bin/flex-fsk-tx -d /dev/ttyACM0 -
```

### Loop mode (multiple messages)

```bash
cat messages.txt | ./bin/flex-fsk-tx -d /dev/ttyACM0 -l -
```

### Custom frequency and power

```bash
./bin/flex-fsk-tx -d /dev/ttyACM0 -f 931.9375 -p 10 1234567 "Custom settings"
```

### Remote encoding (device-side)

```bash
# Device encodes via AT+MSG instead of the host encoding locally with tinyflex
./bin/flex-fsk-tx -d /dev/ttyACM0 -r 1234567 "Device encodes this"
```

Note: the Mail Drop flag (`-m`) only takes effect with local encoding — it is a bit set by the
tinyflex encoder, and there is no AT+MSG-side equivalent in this firmware. `-r -m` together sends
the message without mail drop.

### Configuration wizard

```bash
./bin/flex-fsk-tx --config /dev/ttyUSB0
```

Collects and applies, over the AT interface only: default FLEX settings (capcode, frequency,
power via `AT+FLEX`), and network mode plus WiFi credentials (via `AT+NETWORK`/`AT+WIFI`). Every
setting auto-saves on the device — there is no separate save step. Restarts the device once
applied.

### Factory reset

```bash
./bin/flex-fsk-tx --factoryreset /dev/ttyUSB0
```

## Serial Port Detection

### Linux
- **TTGO LoRa32**: usually `/dev/ttyACM0`
- **Heltec WiFi LoRa 32 V2**: usually `/dev/ttyUSB0`

Check available ports:
```bash
ls /dev/tty* | grep -E "ACM|USB"
# or
dmesg | tail | grep tty
```

### Permission Issues

Add user to dialout group:
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

## Input Format (Stdin Mode)

```
capcode:message
```

Example file:
```
1234567:First message
1234567:Second message
7654321:Different capcode
```

## See Also

- [Web Interface User Guide](../docs/USER_GUIDE.md) (recommended)
- [REST API Documentation](../docs/REST_API.md)
- [AT Command Reference](../docs/AT_COMMANDS.md)
- [Firmware Documentation](../docs/FIRMWARE.md)
