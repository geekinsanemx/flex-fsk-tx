# flex-fsk-tx

FSK transmitter firmware for ESP32 LoRa32 (sx1262) devices with serial AT communications protocol for sending FLEX pager messages.

## Overview

**flex-fsk-tx** is a complete solution for transmitting FLEX pager messages using ESP32 LoRa32 devices. The project consists of two main components:

1. **Host Application** (`flex-fsk-tx.cpp`) - A C++ application that runs on your computer and communicates with the ESP32 device via serial AT commands
2. **Hardware Transmitter Layer** - A dedicated ESP32 LoRa32 hardware device that must be flashed with the custom firmware (`RadioTransmitter_AT.ino`) to handle the actual FSK transmission

## Hardware Requirements

### Required Hardware

- **Primary Support**: [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/) - ESP32-S3 based development board
- **Likely Compatible**: Other Heltec devices using SX1262 LoRa32 chipset (may require pin configuration adjustments)
- **Computer**: Linux/Unix system with serial port support  
- **Connection**: USB cable for ESP32 communication

### Hardware Specifications
- **MCU**: ESP32-S3 (240MHz dual-core)
- **Radio**: SX1262 LoRa/FSK transceiver
- **Frequency Range**: 410-525 MHz / 863-928 MHz (region dependent)
- **TX Power**: Up to +22 dBm
- **Display**: 0.96" OLED display (128x64)
- **Connectivity**: USB-C, WiFi, Bluetooth
- **Manufacturer**: [Heltec Automation](https://heltec.org/)

**Note**: The hardware transmitter layer is a **required dependency** - the host application cannot function without a properly flashed ESP32 device. The firmware must be flashed to the hardware before the system can operate.

## Features

- **FLEX Protocol Support**: Complete FLEX pager message encoding and transmission
- **AT Command Interface**: Standardized AT command protocol for device communication
- **Multiple Input Modes**:
  - Direct command line message transmission
  - Interactive stdin mode with loop support
  - Mail Drop flag support
- **Enhanced Error Handling**: Comprehensive retry logic and error recovery
- **OLED Display**: Real-time status display on ESP32 hardware
- **Power Management**: Automatic display timeout and power saving features
- **Robust Communication**: Serial buffer management and timeout handling
- **Hardware Integration**: Direct control of SX1262 radio chipset for optimal FSK transmission

## Project Components

### 1. Host Application (flex-fsk-tx)

The host application handles FLEX message encoding and communicates with the ESP32 device via AT commands. It supports both single message transmission and continuous operation modes.

### 2. ESP32 Firmware (RadioTransmitter_AT)

The ESP32 firmware provides FSK transmission capabilities with an AT command interface. See [FIRMWARE.md](FIRMWARE.md) for detailed flashing instructions and firmware-specific documentation.

## Project Components

### 1. Host Application (flex-fsk-tx)

The host application handles FLEX message encoding and communicates with the ESP32 device via AT commands. It supports both single message transmission and continuous operation modes.

### 2. Hardware Transmitter Layer (ESP32 Firmware)

The ESP32 hardware device must be flashed with custom firmware (`RadioTransmitter_AT.ino`) that provides:
- FSK transmission capabilities using the SX1262 radio chipset
- AT command interface for host communication
- OLED display for status monitoring
- Power management and error handling

**This is a hardware dependency** - you must have a compatible ESP32 LoRa32 device and flash it with the provided firmware. See [FIRMWARE.md](FIRMWARE.md) for complete hardware setup and flashing instructions.

### Hardware Procurement

To use this system, you will need to purchase a compatible ESP32 LoRa32 development board:

**Recommended Hardware**: [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/)
- **Manufacturer**: [Heltec Automation](https://heltec.org/)
- **Official Store**: [Heltec Store](https://heltec.org/proudct_center/lora/)
- **Third-party Availability**: Available through major electronics distributors (Digi-Key, Mouser, Amazon, AliExpress, etc.)

**Alternative Compatible Hardware**: Other Heltec devices with SX1262 chipset may work with pin configuration adjustments.

## Quick Start

### Prerequisites

Before starting, ensure you have:
1. **Hardware**: A Heltec WiFi LoRa 32 V3 development board (or compatible)
2. **Computer**: Linux/Unix system with USB port and development tools
3. **Antenna**: Appropriate antenna for your frequency band
4. **USB Cable**: For connecting the ESP32 device

### 1. Acquire Hardware

Purchase a [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/) development board from:
- [Official Heltec Store](https://heltec.org/proudct_center/lora/)
- Electronics distributors (Digi-Key, Mouser, etc.)
- Online marketplaces (Amazon, AliExpress, etc.)

### 2. Clone the Repository

**Important**: Use the `--recursive` flag to download the required tinyflex dependency library:

```bash
git clone --recursive https://github.com/geekinsanemx/flex-fsk-tx.git
cd flex-fsk-tx
```

### 2. Flash the Firmware

Flash the `RadioTransmitter_AT.ino` firmware to your ESP32 device. See [FIRMWARE.md](FIRMWARE.md) for detailed instructions.

### 3. Build the Host Application

```bash
make
sudo make install
```

### 4. Send Your First Message

```bash
# Basic usage
flex-fsk-tx 1234567 "Hello World"

# Custom frequency and power
flex-fsk-tx -f 931.9375 -p 10 1234567 "Test Message"

# Interactive mode from stdin
echo "1234567:Hello from stdin" | flex-fsk-tx -

# Loop mode for multiple messages
printf "1234567:Message 1\n8901234:Message 2\n" | flex-fsk-tx -l -
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
| `AT+STATUS?` | Query | None | `+STATUS: <state>`<br>`OK` | Query current device status |
| `AT+ABORT` | Execute | None | `OK` | Abort current operation |
| `AT+RESET` | Execute | None | `OK` (then restart) | Software reset device |

### Response Codes

| Response | Description |
|----------|-------------|
| `OK` | Command executed successfully |
| `ERROR` | Command failed or invalid parameter |
| `+SEND: READY` | Device ready to receive binary data |
| `+FREQ: <value>` | Current frequency in MHz |
| `+POWER: <value>` | Current power in dBm |
| `+STATUS: <state>` | Current device state |

### Device Status States

| Status | Description |
|--------|-------------|
| `READY` | Device idle and ready for commands |
| `WAITING_DATA` | Device waiting for binary data after AT+SEND |
| `TRANSMITTING` | Device currently transmitting data |
| `ERROR` | Device in error state |

### Command Format Rules

- All commands must end with `\r\n` (carriage return + line feed)
- Commands are case-insensitive
- Parameters are separated by `=` for set commands
- Query commands end with `?`
- Responses are terminated with `\r\n`

### Data Transmission Protocol

| Step | Host → Device | Device → Host | Description |
|------|---------------|---------------|-------------|
| 1 | `AT+SEND=<length>\r\n` | | Request to send data |
| 2 | | `+SEND: READY\r\n` | Device ready for binary data |
| 3 | `<binary_data>` | | Send raw binary data (no termination) |
| 4 | | `OK\r\n` | Transmission completed successfully |

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
# Send a simple message
flex-fsk-tx 1234567 "Your message here"

# Use custom serial device
flex-fsk-tx -d /dev/ttyUSB0 1234567 "Test message"

# Set custom frequency and power
flex-fsk-tx -f 916.0 -p 15 1234567 "High power message"

# Enable Mail Drop flag
flex-fsk-tx -m 1234567 "Mail Drop message"
```

### Stdin Mode

```bash
# Single message from stdin
echo "1234567:Hello World" | flex-fsk-tx -

# Multiple messages in loop mode
printf "1234567:Message 1\n8901234:Message 2\n" | flex-fsk-tx -l -

# Mail Drop with loop mode
printf "1234567:Important\n8901234:Urgent\n" | flex-fsk-tx -l -m -
```

### Configuration Options

```
-d <device>    Serial device (default: /dev/ttyACM0)
-b <baudrate>  Baudrate (default: 115200)
-f <frequency> Frequency in MHz (default: 916.0)
-p <power>     TX power in dBm (default: 2, range: 2-20)
-l             Loop mode: continuous operation until EOF
-m             Mail Drop: sets Mail Drop Flag in FLEX message
```

## Device Status Display

The ESP32 firmware provides real-time status information on the built-in OLED display:

- **Banner**: Device identification
- **State**: Current operation status (Ready, Receiving Data, Transmitting, Error)
- **TX Power**: Current transmission power setting
- **Frequency**: Current transmission frequency

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
- **Frequency Range**: 400-1000 MHz
- **Power Range**: -9 to 22 dBm
- **Serial Baudrate**: 115200 bps
- **Maximum Message Length**: Varies by FLEX protocol specifications

## Troubleshooting

### Common Issues

1. **Device Not Responding**
   - Check USB connection and serial device path
   - Verify ESP32 firmware is properly flashed
   - Try different baudrate or serial device

2. **Transmission Failures**
   - Ensure antenna is properly connected
   - Check frequency and power settings
   - Verify device is not in error state

3. **Permission Denied**
   - Add user to dialout group: `sudo usermod -a -G dialout $USER`
   - Log out and back in for changes to take effect

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
