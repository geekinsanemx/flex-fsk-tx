# flex-fsk-tx

FSK transmitter firmware for ESP32 LoRa32 (sx1262/sx1276) devices with serial AT communications protocol for sending FLEX pager messages.

## Overview

**flex-fsk-tx** is a complete solution for transmitting FLEX pager messages using ESP32 LoRa32 devices. The project consists of two main components:

1. **Host Application** (`flex-fsk-tx.cpp`) - A C++ application that runs on your computer and communicates with the ESP32 device via serial AT commands
2. **Hardware Transmitter Layer** - A dedicated ESP32 LoRa32 hardware device that must be flashed with the custom firmware to handle the actual FSK transmission

## Hardware Requirements

### Supported Hardware

- **Tested**: [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/) - ESP32-S3 based development board with SX1262 chipset
- **Tested**: [TTGO LoRa32-OLED](https://lilygo.cc/products/lora3) - ESP32 based development board with SX1276 chipset
- **Likely Compatible**: Other ESP32 devices using SX1262/SX1276 LoRa chipsets (may require pin configuration adjustments)
- **Computer**: Linux/Unix system with serial port support  
- **Connection**: USB cable for ESP32 communication

### Hardware Specifications

**Heltec WiFi LoRa 32 V3**
- **MCU**: ESP32-S3 (240MHz dual-core)
- **Radio**: SX1262 LoRa/FSK transceiver
- **Frequency Range**: 433 MHz / 868 MHz / 915 MHz (region dependent)
- **TX Power**: Up to +22 dBm
- **Display**: 0.96" OLED display (128x64)
- **Connectivity**: USB-C, WiFi, Bluetooth
- **Serial Port**: Typically `/dev/ttyUSB0` on Linux
- **Manufacturer**: [Heltec Automation](https://heltec.org/)

**TTGO LoRa32-OLED**
- **MCU**: ESP32 (240MHz dual-core)
- **Radio**: SX1276 LoRa/FSK transceiver
- **Frequency Range**: 433 MHz / 868 MHz / 915 MHz (region dependent)
- **TX Power**: Up to +20 dBm
- **Display**: 0.96" OLED display (128x64)
- **Connectivity**: USB-C, WiFi, Bluetooth
- **Serial Port**: Typically `/dev/ttyACM0` on Linux
- **Manufacturer**: LilyGO (TTGO)

**Note**: The hardware transmitter layer is a **required dependency** - the host application cannot function without a properly flashed ESP32 device. The firmware must be flashed to the hardware before the system can operate.

## Features

- **FLEX Protocol Support**: Complete FLEX pager message encoding and transmission
- **Dual Encoding Modes**:
  - **Local Encoding (default)**: FLEX messages encoded on host using tinyflex library
  - **Remote Encoding (-r flag)**: On-device FLEX encoding using ESP32's tinyflex integration (v2 firmware)
- **AT Command Interface**: Standardized AT command protocol for device communication
- **Multiple Hardware Support**: Compatible with both Heltec V3 and TTGO LoRa32-OLED devices
- **Multiple Input Modes**:
  - Direct command line message transmission
  - Interactive stdin mode with loop support
  - Mail Drop flag support
- **Enhanced Error Handling**: Comprehensive retry logic and error recovery
- **OLED Display**: Real-time status display on ESP32 hardware
- **Power Management**: Automatic display timeout and power saving features
- **Robust Communication**: Serial buffer management and timeout handling
- **Hardware Integration**: Direct control of SX1262/SX1276 radio chipset for optimal FSK transmission

## Project Components

### 1. Host Application (flex-fsk-tx)

The host application handles FLEX message encoding and communicates with the ESP32 device via AT commands. It supports both single message transmission and continuous operation modes.

### 2. Hardware Transmitter Layer (ESP32 Firmware)

The ESP32 hardware device must be flashed with custom firmware that provides:
- FSK transmission capabilities using the SX1262 or SX1276 radio chipset
- AT command interface for host communication
- FLEX message encoding (v2 firmware versions)
- OLED display for status monitoring
- Power management and error handling

**This is a hardware dependency** - you must have a compatible ESP32 LoRa32 device and flash it with the provided firmware.

#### Firmware Locations by Device:

- **Heltec WiFi LoRa 32 V3**:
  - Basic firmware: See [FIRMWARE.md](FIRMWARE.md) for complete hardware setup and flashing instructions
  - **v2 firmware with on-device encoding**: `Devices/Heltec LoRa32 V3/heltec_fsk_tx_AT_v2.ino`
- **TTGO LoRa32-OLED**:
  - Basic firmware: See [FIRMWARE.md](FIRMWARE.md) for detailed instructions
  - **v2 firmware with on-device encoding**: `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT_v2.ino`

#### Firmware Versions

**v1 Firmware (Original)**:
- Requires host application to encode FLEX messages
- Supports binary data transmission via `AT+SEND` command
- Smaller memory footprint

**v2 Firmware (Recommended)**:
- **On-device FLEX encoding** - ESP32 encodes messages directly
- Supports both binary data (`AT+SEND`) and text messages (`AT+MSG`)
- Includes tinyflex library integration
- Enhanced functionality with mail drop support
- Backward compatible with v1 host applications

### Hardware Procurement

To use this system, you will need to purchase a compatible ESP32 LoRa32 development board:

**Heltec WiFi LoRa 32 V3** (Recommended):
- **Manufacturer**: [Heltec Automation](https://heltec.org/)
- **Official Store**: [Heltec Store](https://heltec.org/proudct_center/lora/)
- **Third-party Availability**: Available through major electronics distributors (Digi-Key, Mouser, Amazon, AliExpress, etc.)

**TTGO LoRa32-OLED** (Alternative):
- **Manufacturer**: LilyGO (TTGO)
- **Availability**: Available through AliExpress, Banggood, Amazon, and other electronics retailers
- **Note**: Ensure you get the version with OLED display for full functionality

## Quick Start

### Prerequisites

Before starting, ensure you have:
1. **Hardware**: A compatible ESP32 LoRa32 development board (Heltec V3 or TTGO LoRa32-OLED)
2. **Computer**: Linux/Unix system with USB port and development tools
3. **Antenna**: Appropriate antenna for your frequency band
4. **USB Cable**: For connecting the ESP32 device

### 1. Acquire Hardware

Purchase either:
- [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/) from official or authorized retailers
- [TTGO LoRa32-OLED](https://lilygo.cc/products/lora3) from electronics marketplaces

### 2. Clone the Repository

**Important**: Use the `--recursive` flag to download the required tinyflex dependency library:

```bash
git clone --recursive https://github.com/geekinsanemx/flex-fsk-tx.git
cd flex-fsk-tx
```

### 3. Flash the Firmware

Flash the appropriate firmware to your ESP32 device:

- **Heltec WiFi LoRa 32 V3**:
  - Basic: See [FIRMWARE.md](FIRMWARE.md) for detailed instructions
  - **v2 with on-device encoding**: Flash `Devices/Heltec LoRa32 V3/heltec_fsk_tx_AT_v2.ino`
- **TTGO LoRa32-OLED**:
  - Basic: See [FIRMWARE.md](FIRMWARE.md) for detailed instructions
  - **v2 with on-device encoding**: Flash `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT_v2.ino`

### 4. Build the Host Application

```bash
make
sudo make install
```

### 5. Send Your First Message

```bash
# For Heltec WiFi LoRa 32 V3 (typically /dev/ttyUSB0)
# Local encoding (default)
flex-fsk-tx -d /dev/ttyUSB0 1234567 "Hello World"

# Remote encoding (v2 firmware required)
flex-fsk-tx -d /dev/ttyUSB0 -r 1234567 "Hello World"

# For TTGO LoRa32-OLED (typically /dev/ttyACM0)  
# Local encoding
flex-fsk-tx -d /dev/ttyACM0 1234567 "Hello World"

# Remote encoding (v2 firmware required)
flex-fsk-tx -d /dev/ttyACM0 -r 1234567 "Hello World"

# Custom frequency and power with remote encoding
flex-fsk-tx -d /dev/ttyUSB0 -r -f 931.9375 -p 10 1234567 "Test Message"

# Interactive mode from stdin with remote encoding
echo "1234567:Hello from stdin" | flex-fsk-tx -d /dev/ttyACM0 -r -

# Loop mode for multiple messages with remote encoding
printf "1234567:Message 1\n8901234:Message 2\n" | flex-fsk-tx -d /dev/ttyUSB0 -l -r -
```

## AT Command Protocol

The communication between the host application and ESP32 firmware uses a standardized AT command protocol based on the Hayes command set.

### AT Command Reference Table

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT` | Test | None | `OK` | Test communication and reset device state |
| `AT+FREQ=<value>` | Set | `<value>`: 400.0-1000.0 (MHz) | `OK` / `ERROR` | Set transmission frequency |
| `AT+FREQ?` | Query | None | `+FREQ: <value>`<br>`OK` | Query current frequency setting |
| `AT+POWER=<value>` | Set | `<value>`: -9 to 22 (dBm) | `OK` / `ERROR` | Set transmission power |
| `AT+POWER?` | Query | None | `+POWER: <value>`<br>`OK` | Query current power setting |
| `AT+SEND=<length>` | Execute | `<length>`: 1-2048 (bytes) | `+SEND: READY` | Initiate binary data transmission |
| `AT+MSG=<capcode>` | Execute | `<capcode>`: Target capcode | `+MSG: READY` | Send FLEX message (v2 firmware only) |
| `AT+MAILDROP=<value>` | Set | `<value>`: 0 or 1 | `OK` / `ERROR` | Set Mail Drop flag (v2 firmware only) |
| `AT+MAILDROP?` | Query | None | `+MAILDROP: <value>`<br>`OK` | Query Mail Drop flag (v2 firmware only) |
| `AT+STATUS?` | Query | None | `+STATUS: <state>`<br>`OK` | Query current device status |
| `AT+ABORT` | Execute | None | `OK` | Abort current operation |
| `AT+RESET` | Execute | None | `OK` (then restart) | Software reset device |

### Response Codes

| Response | Description |
|----------|-------------|
| `OK` | Command executed successfully |
| `ERROR` | Command failed or invalid parameter |
| `+SEND: READY` | Device ready to receive binary data |
| `+MSG: READY` | Device ready to receive text message (v2 firmware only) |
| `+FREQ: <value>` | Current frequency in MHz |
| `+POWER: <value>` | Current power in dBm |
| `+MAILDROP: <value>` | Current Mail Drop flag setting (v2 firmware only) |
| `+STATUS: <state>` | Current device state |

### Device Status States

| Status | Description |
|--------|-------------|
| `READY` | Device idle and ready for commands |
| `WAITING_DATA` | Device waiting for binary data after AT+SEND |
| `WAITING_MSG` | Device waiting for text message after AT+MSG (v2 firmware only) |
| `TRANSMITTING` | Device currently transmitting data |
| `ERROR` | Device in error state |

### Command Format Rules

- All commands must end with `\r\n` (carriage return + line feed)
- Commands are case-insensitive
- Parameters are separated by `=` for set commands
- Query commands end with `?`
- Responses are terminated with `\r\n`

### Data Transmission Protocols

#### Binary Data Transmission (AT+SEND)

| Step | Host → Device | Device → Host | Description |
|------|---------------|---------------|-------------|
| 1 | `AT+SEND=<length>\r\n` | | Request to send binary data |
| 2 | | `+SEND: READY\r\n` | Device ready for binary data |
| 3 | `<binary_data>` | | Send raw binary data (no termination) |
| 4 | | `OK\r\n` | Transmission completed successfully |

#### FLEX Message Transmission (AT+MSG) - v2 Firmware Only

| Step | Host → Device | Device → Host | Description |
|------|---------------|---------------|-------------|
| 1 | `AT+MSG=<capcode>\r\n` | | Request to send FLEX message |
| 2 | | `+MSG: READY\r\n` | Device ready for text message |
| 3 | `<text_message>\r\n` | | Send text message (terminated with \r\n) |
| 4 | | `OK\r\n` | Message encoded and transmitted successfully |

### Error Handling

| Scenario | Device Response | Host Action |
|----------|----------------|-------------|
| Invalid command | `ERROR\r\n` | Retry or abort |
| Parameter out of range | `ERROR\r\n` | Check parameter limits |
| Device busy | `ERROR\r\n` | Wait and retry |
| Transmission timeout | `ERROR\r\n` | Reset and retry |
| Communication lost | No response | Send `AT` to test connection |

## Usage Examples

### Command Line Mode

```bash
# Send a simple message (local encoding - default)
flex-fsk-tx 1234567 "Your message here"

# Send message with remote encoding (v2 firmware required)
flex-fsk-tx -r 1234567 "Your message here"

# Use specific serial device for Heltec V3 (local encoding)
flex-fsk-tx -d /dev/ttyUSB0 1234567 "Test message"

# Use specific serial device for TTGO LoRa32-OLED (remote encoding)
flex-fsk-tx -d /dev/ttyACM0 -r 1234567 "Test message"

# Set custom frequency and power with remote encoding
flex-fsk-tx -d /dev/ttyUSB0 -f 916.0 -p 15 -r 1234567 "High power message"

# Enable Mail Drop flag (local encoding)
flex-fsk-tx -d /dev/ttyACM0 -m 1234567 "Mail Drop message"

# Enable Mail Drop flag with remote encoding
flex-fsk-tx -d /dev/ttyACM0 -r -m 1234567 "Remote Mail Drop message"
```

### Stdin Mode

```bash
# Single message from stdin (local encoding)
echo "1234567:Hello World" | flex-fsk-tx -d /dev/ttyUSB0 -

# Single message from stdin (remote encoding)
echo "1234567:Hello World" | flex-fsk-tx -d /dev/ttyUSB0 -r -

# Multiple messages in loop mode (local encoding)
printf "1234567:Message 1\n8901234:Message 2\n" | flex-fsk-tx -d /dev/ttyACM0 -l -

# Multiple messages in loop mode (remote encoding)
printf "1234567:Message 1\n8901234:Message 2\n" | flex-fsk-tx -d /dev/ttyACM0 -l -r -

# Mail Drop with loop mode and remote encoding
printf "1234567:Important\n8901234:Urgent\n" | flex-fsk-tx -d /dev/ttyUSB0 -l -m -r -
```

### Device-Specific Examples

```bash
# For Heltec WiFi LoRa 32 V3 (local encoding):
bin/flex-fsk-tx -d /dev/ttyUSB0 1234567 'MY MESSAGE'

# For TTGO LoRa32-OLED (remote encoding):
bin/flex-fsk-tx -d /dev/ttyACM0 -r 1234567 'MY MESSAGE'

# Custom frequency with remote encoding:
bin/flex-fsk-tx -d /dev/ttyUSB0 -f 915.5 -r 1234567 'MY MESSAGE'
```

### Normal Mode Examples

```bash
# Basic message (local encoding)
bin/flex-fsk-tx 1234567 'MY MESSAGE'

# Mail Drop (local encoding)
bin/flex-fsk-tx -m 1234567 'MY MESSAGE'

# Remote encoding
bin/flex-fsk-tx -r 1234567 'MY MESSAGE'

# Remote encoding with Mail Drop
bin/flex-fsk-tx -r -m 1234567 'MY MESSAGE'
```

### Stdin Mode Examples

```bash
# Basic stdin
printf '1234567:MY MESSAGE' | bin/flex-fsk-tx -

# Multiple messages with loop mode
printf '1234567:MY MSG1\n1122334:MY MSG2' | bin/flex-fsk-tx -l -

# Mail Drop from stdin
printf '1234567:MY MESSAGE' | bin/flex-fsk-tx -m -

# Remote encoding from stdin
printf '1234567:MY MESSAGE' | bin/flex-fsk-tx -r -

# All options combined
printf '1234567:MY MESSAGE' | bin/flex-fsk-tx -l -m -r -
```

### Direct AT Commands (v2 Firmware)

With v2 firmware, you can also send AT commands directly for on-device FLEX encoding:

```bash
# Connect to device
screen /dev/ttyUSB0 115200

# Set frequency and power
AT+FREQ=931.9375
AT+POWER=10

# Send FLEX message with on-device encoding
AT+MSG=1234567
# Device responds: +MSG: READY
# Type your message and press Enter:
Hello from direct AT commands!
# Device responds: OK (message transmitted)

# Enable Mail Drop for next message
AT+MAILDROP=1
AT+MSG=8901234
# Device responds: +MSG: READY
Important mail drop message
# Device responds: OK

# Check status
AT+STATUS?
# Device responds: +STATUS: READY
```

### Configuration Options

```
bin/flex-fsk-tx [options] <capcode> <message>
or:
bin/flex-fsk-tx [options] [-l] [-m] [-r] - (from stdin)

Options:
   -d <device>    Serial device (default: /dev/ttyUSB0)
                  Common devices:
                  /dev/ttyUSB0 - Heltec WiFi LoRa 32 V3
                  /dev/ttyACM0 - TTGO LoRa32-OLED
   -b <baudrate>  Baudrate (default: 115200)
   -f <frequency> Frequency in MHz (default: 916.000000)
   -p <power>     TX power (default: 2, 2-20)
   -l             Loop mode: stays open receiving new lines until EOF
   -m             Mail Drop: sets the Mail Drop Flag in the FLEX message
   -r             Remote encoding: use device's AT+MSG command instead of
                  local encoding. Encoding is performed on the device.

Encoding modes:
   Default (local):  Encode FLEX message on host using tinyflex library,
                     then send binary data with AT+SEND command
   Remote (-r):      Send capcode and message text to device using
                     AT+MSG command for device-side encoding
```

## Device Status Display

Both supported ESP32 firmwares provide real-time status information on the built-in OLED display:

- **Banner**: Device identification ("GeekInsaneMX")
- **State**: Current operation status (Ready, Receiving Data, Receiving Msg, Transmitting, Error)
- **TX Power**: Current transmission power setting
- **Frequency**: Current transmission frequency

The v2 firmware displays additional states:
- **Receiving Msg...**: When waiting for text message input via AT+MSG command
- Enhanced status tracking for on-device FLEX encoding

## Error Handling and Recovery

The system includes comprehensive error handling:

- **Automatic Retries**: Failed commands are automatically retried with exponential backoff
- **Timeout Management**: Configurable timeouts prevent system hangs
- **Buffer Management**: Automatic serial buffer flushing and overflow protection
- **State Recovery**: Device state reset on communication errors
- **Visual Feedback**: LED and display status indicators

## Technical Specifications

- **Transmission Mode**: FSK (Frequency Shift Keying)
- **Bit Rate**: 1.6 kbps
- **Frequency Deviation**: 5 kHz
- **Receive Bandwidth**: 10.4 kHz
- **Frequency Range**: 400-1000 MHz (device dependent)
- **Power Range**: -9 to 22 dBm (device dependent)
- **Serial Baudrate**: 115200 bps
- **Maximum Message Length**:
  - Binary mode (AT+SEND): 2048 bytes
  - FLEX message mode (AT+MSG): 240 characters (v2 firmware)

## Troubleshooting

### Common Issues

1. **Device Not Responding**
   - Check USB connection and serial device path
   - Try `/dev/ttyUSB0` for Heltec devices or `/dev/ttyACM0` for TTGO devices
   - Verify ESP32 firmware is properly flashed
   - Try different baudrate or serial device
   - Ensure you're using the correct firmware version (v1 vs v2)

2. **AT+MSG Command Not Recognized**
   - Verify you're using v2 firmware with on-device encoding support
   - Flash the appropriate v2 firmware from the `Devices/` directory
   - Check firmware version compatibility

3. **Transmission Failures**
   - Ensure antenna is properly connected
   - Check frequency and power settings
   - Verify device is not in error state
   - For v2 firmware, ensure FLEX message is within 240 character limit

4. **Permission Denied**
   - Add user to dialout group: `sudo usermod -a -G dialout $USER`
   - Log out and back in for changes to take effect

5. **Serial Port Detection**
   - List available ports: `ls /dev/tty*`
   - Check dmesg when plugging device: `dmesg | tail`
   - Common ports:
     - `/dev/ttyUSB0` - Heltec WiFi LoRa 32 V3
     - `/dev/ttyACM0` - TTGO LoRa32-OLED

## Acknowledgments

This project is based on the excellent work of:

- **Davidson Francis (Theldus)** and **Rodrigo Laneth** - Original tinyflex library and send_ttgo application
  - [tinyflex](https://github.com/Theldus/tinyflex) - FLEX protocol implementation
  - [ttgo-fsk-tx](https://github.com/rlaneth/ttgo-fsk-tx/) - Original ESP32 FSK transmitter firmware

Special thanks to these developers for providing the foundation that made this standardized AT command implementation possible.

## License

This project is released into the public domain. This is free and unencumbered software released into the public domain.

## Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.

## Support

For support, please open an issue on the GitHub repository or refer to the documentation in the `docs/` directory.
