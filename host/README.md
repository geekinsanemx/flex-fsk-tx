# flex-fsk-tx Host Application (Optional)

## Overview

This C++ application provides **optional** serial communication with ESP32 devices using AT commands.

**⚠️ NOT REQUIRED FOR NORMAL OPERATION**

The v3+ firmware includes:
- ✅ **Web Interface** (port 80) - Primary user interface
- ✅ **REST API** (port 16180) - Programmatic control
- ✅ **MQTT Integration** - IoT messaging
- ✅ **IMAP Integration** - Email-to-pager

## When to Use This Tool

**Use the host application if:**
- Testing AT commands during firmware development
- Operating device without WiFi (serial-only mode)
- Batch processing messages via stdin
- Integrating with legacy systems requiring serial control
- Using v1/v2 firmware (no web interface)

**Use the web interface instead if:**
- Normal message transmission
- Device configuration
- IMAP/MQTT/ChatGPT setup
- System monitoring

## Build & Install

```bash
# Build
make

# Install system-wide (optional)
sudo make install

# Clean build artifacts
make clean

# Debug build
make debug

# Check dependencies
make check-deps
```

## Prerequisites

- C++ compiler (g++)
- tinyflex library (submodule in `../include/tinyflex/`)
- Serial port access permissions

## Usage Examples

### Single Message

```bash
./bin/flex-fsk-tx -d /dev/ttyACM0 1234567 "Test message"
```

### Stdin Mode

```bash
echo "1234567:Hello World" | ./bin/flex-fsk-tx -d /dev/ttyACM0 -
```

### Loop Mode (Multiple Messages)

```bash
cat messages.txt | ./bin/flex-fsk-tx -d /dev/ttyACM0 -l -
```

### Custom Frequency and Power

```bash
./bin/flex-fsk-tx -d /dev/ttyACM0 -f 929.6625 -p 10 1234567 "Custom settings"
```

### Remote Encoding (v2+ Firmware)

```bash
# Let device encode FLEX message (saves bandwidth)
./bin/flex-fsk-tx -d /dev/ttyACM0 -r 1234567 "Device encodes this"
```

## Command Line Options

```
Usage: flex-fsk-tx [options] <capcode> <message>
       flex-fsk-tx [options] -

Options:
  -d <device>     Serial device (default: /dev/ttyUSB0)
  -f <freq>       Frequency in MHz (default: 916.0)
  -p <power>      TX power in dBm (default: 2)
  -b <baudrate>   Serial baudrate (default: 115200)
  -r              Remote encoding (use AT+MSG on v2+ firmware)
  -l              Loop mode (multiple messages from stdin)
  -               Read from stdin (format: capcode:message)
```

## Serial Port Detection

### Linux
- **TTGO LoRa32**: Usually `/dev/ttyACM0`
- **Heltec V2**: Usually `/dev/ttyUSB0`

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
