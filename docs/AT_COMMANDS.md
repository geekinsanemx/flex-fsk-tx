# FLEX Paging Message Transmitter - AT Commands Guide

Complete guide for using the AT command interface to control the FLEX paging message transmitter via serial communication.

> **Note**: AT commands are available in all firmware versions (v1, v2, v3). This guide includes WiFi and advanced commands specific to v3 firmware.

## 🔗 Connection Setup

### Hardware Connection
- **Interface**: USB Serial (Virtual COM Port)
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

### Device Serial Port Identification

| Device | Linux Port | Windows Port | Notes |
|--------|-----------|--------------|-------|
| **TTGO LoRa32** | `/dev/ttyACM0` | `COM3+` | ESP32 + SX1276, fully supported |
| **Heltec WiFi LoRa32 V2** | `/dev/ttyUSB0` | `COM4+` | ESP32 + SX1276, fully supported |

### Software Options

#### Arduino IDE Serial Monitor
1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Set line ending to **Both NL & CR**
4. Type commands in the input field

#### PuTTY (Windows)
```
Connection Type: Serial
Serial Line: COM3 (TTGO) or COM4+ (Heltec)
Speed: 115200
Data bits: 8
Stop bits: 1
Parity: None
Flow control: None
```

#### Screen (Linux/macOS)
```bash
# TTGO LoRa32
screen /dev/ttyACM0 115200

# Heltec WiFi LoRa32 V2
screen /dev/ttyUSB0 115200
```

#### Minicom (Linux)
```bash
# TTGO LoRa32
minicom -b 115200 -D /dev/ttyACM0

# Heltec WiFi LoRa32 V2
minicom -b 115200 -D /dev/ttyUSB0
```

#### Python Terminal
```python
import serial

# TTGO LoRa32
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

# Heltec WiFi LoRa32 V2
# ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

ser.write(b'AT\r\n')
print(ser.readline().decode())
```

## 📋 Complete AT Commands Reference

### Basic Commands

| Command | Type | Parameters | Response | Firmware | Description |
|---------|------|------------|----------|----------|-------------|
| `AT` | Test | None | `OK` | v1,v2,v3 | Test communication and reset device state |
| `AT+STATUS?` | Query | None | `+STATUS: <state>`<br>`OK` | v1,v2,v3 | Query current device status |
| `AT+ABORT` | Execute | None | `OK` | v1,v2,v3 | Abort current operation |
| `AT+RESET` | Execute | None | `OK` (then restart) | v1,v2,v3 | Software reset device |

### Radio Configuration Commands

| Command | Type | Parameters | Response | Firmware | Description |
|---------|------|------------|----------|----------|-------------|
| `AT+FREQ=<value>` | Set | `<value>`: 400.0-1000.0 (MHz) | `OK` / `ERROR` | v1,v2,v3 | Set transmission frequency |
| `AT+FREQ?` | Query | None | `+FREQ: <value>`<br>`OK` | v1,v2,v3 | Query current frequency setting |
| `AT+FREQPPM=<value>` | Set | `<value>`: -50.0 to +50.0 (PPM) | `OK` / `ERROR` | v1,v2,v3 | Set frequency correction in PPM |
| `AT+FREQPPM?` | Query | None | `+FREQPPM: <value>`<br>`OK` | v1,v2,v3 | Query current frequency correction |
| `AT+POWER=<value>` | Set | `<value>`: 0 to 20 (dBm) | `OK` / `ERROR` | v1,v2,v3 | Set transmission power |
| `AT+POWER?` | Query | None | `+POWER: <value>`<br>`OK` | v1,v2,v3 | Query current power setting |

### Message Transmission Commands

| Command | Type | Parameters | Response | Firmware | Description |
|---------|------|------------|----------|----------|-------------|
| `AT+SEND=<length>` | Execute | `<length>`: 1-2048 (bytes) | `+SEND: READY` | v1,v2,v3 | Initiate binary data transmission |
| `AT+MSG=<capcode>` | Execute | `<capcode>`: Target capcode | `+MSG: READY` | v2,v3 | Send FLEX message (on-device encoding) |
| `AT+MAILDROP=<value>` | Set | `<value>`: 0 or 1 | `OK` / `ERROR` | v2,v3 | Set Mail Drop flag |
| `AT+MAILDROP?` | Query | None | `+MAILDROP: <value>`<br>`OK` | v2,v3 | Query Mail Drop flag setting |

### WiFi Commands (v3 Firmware Only)

| Command | Type | Parameters | Response | Firmware | Description |
|---------|------|------------|----------|----------|-------------|
| `AT+WIFI?` | Query | None | `+WIFI: <status>` | v3 | Query WiFi connection status |
| `AT+WIFI=<ssid>,<pass>` | Set | SSID and password | `OK` / `ERROR` | v3 | Configure and connect to WiFi |
| `AT+WIFICONFIG?` | Query | None | Configuration details | v3 | Show current WiFi configuration |
| `AT+WIFIENABLE=<value>` | Set | `<value>`: 0 or 1 | `OK` / `ERROR` | v3 | Enable/disable WiFi functionality |
| `AT+WIFIENABLE?` | Query | None | `+WIFIENABLE: <value>` | v3 | Query WiFi enable status |

### API Configuration Commands (v3 Firmware Only)

| Command | Type | Parameters | Response | Firmware | Description |
|---------|------|------------|----------|----------|-------------|
| `AT+APIPORT=<port>` | Set | `<port>`: 1024-65535 | `OK` / `ERROR` | v3 | Set REST API port |
| `AT+APIPORT?` | Query | None | `+APIPORT: <port>` | v3 | Query REST API port |
| `AT+APIUSER=<username>` | Set | Username (1-32 chars) | `OK` / `ERROR` | v3 | Set API authentication username |
| `AT+APIUSER?` | Query | None | `+APIUSER: <username>` | v3 | Query API username |
| `AT+APIPASS=<password>` | Set | Password (1-64 chars) | `OK` / `ERROR` | v3 | Set API authentication password |
| `AT+APIPASS?` | Query | None | `+APIPASS: ***` | v3 | Query API password (masked) |

### Device Configuration Commands (v3 Firmware Only)

| Command | Type | Parameters | Response | Firmware | Description |
|---------|------|------------|----------|----------|-------------|
| `AT+BANNER=<text>` | Set | Text (1-16 chars) | `OK` / `ERROR` | v3 | Set custom banner message |
| `AT+BANNER?` | Query | None | `+BANNER: <text>` | v3 | Query current banner |
| `AT+BATTERY?` | Query | None | `+BATTERY: <voltage>,<percent>` | v3 | Query battery status |
| `AT+SAVE` | Execute | None | `OK` / `ERROR` | v3 | Save configuration to NVS |
| `AT+FACTORYRESET` | Execute | None | `OK` (then restart) | v3 | Reset to factory defaults |

## 🔄 Device Status States

| Status | Description |
|--------|-------------|
| `READY` | Device idle and ready for commands |
| `WAITING_DATA` | Device waiting for binary data after AT+SEND |
| `WAITING_MSG` | Device waiting for text message after AT+MSG |
| `TRANSMITTING` | Device currently transmitting data |
| `ERROR` | Device in error state |
| `WIFI_CONNECTING` | WiFi connection in progress (v3 only) |
| `WIFI_AP_MODE` | Access Point mode active (v3 only) |

**Note**: v3 firmware includes a message queue system for the web interface and REST API that can queue up to 25 messages automatically, reducing the frequency of "device busy" scenarios.

## 📡 Command Usage Examples

### Basic Operation

```bash
# Test connection
AT

# Check device status
AT+STATUS?

# Set frequency to 929.6625 MHz (common FLEX frequency)
AT+FREQ=929.6625

# Set transmit power to 10 dBm
AT+POWER=10

# Query current settings
AT+FREQ?
AT+POWER?

# Set frequency correction
# Example: 4.3 PPM correction for observed 4kHz offset at 932MHz
AT+FREQPPM=4.3

# Query frequency correction
AT+FREQPPM?
```

### Binary Data Transmission (All Firmware Versions)

```bash
# Send 10 bytes of binary data
AT+SEND=10
# Wait for "+SEND: READY" response
# Then send exactly 10 bytes of binary data
# Device responds with "OK" when complete
```

### FLEX Message Transmission (v2+ Firmware)

```bash
# Enable mail drop flag
AT+MAILDROP=1

# Send FLEX message to capcode 1234567
AT+MSG=1234567
# Wait for "+MSG: READY" response
# Type your message and press Enter:
Hello World!
# Device responds with "OK" when transmitted
```

### WiFi Configuration (v3 Firmware)

```bash
# Configure WiFi network
AT+WIFI=MyNetwork,MyPassword

# Check WiFi status
AT+WIFI?
# Response: +WIFI: CONNECTED,192.168.1.100

# Show WiFi configuration
AT+WIFICONFIG?

# Disable WiFi
AT+WIFIENABLE=0
```

### REST API Configuration (v3 Firmware)

```bash
# Set API port to 8080
AT+APIPORT=8080

# Set API credentials
AT+APIUSER=admin
AT+APIPASS=secretpassword

# Save configuration
AT+SAVE
```

### Device Customization (v3 Firmware)

```bash
# Set custom banner (max 16 characters)
AT+BANNER=My FLEX TX

# Check battery status
AT+BATTERY?
# Response: +BATTERY: 4.12V,85%

# Save all settings to NVS
AT+SAVE

# Factory reset (restores all defaults)
AT+FACTORYRESET
```

## 🚨 Error Handling

### Response Codes

| Response | Description |
|----------|-------------|
| `OK` | Command executed successfully |
| `ERROR` | Command failed or invalid parameter |
| `+SEND: READY` | Device ready to receive binary data |
| `+MSG: READY` | Device ready to receive text message |

### Common Error Scenarios

| Scenario | Device Response | Suggested Action |
|----------|----------------|------------------|
| Invalid command | `ERROR` | Check command syntax |
| Parameter out of range | `ERROR` | Verify parameter limits |
| Device busy | `ERROR` | Wait and retry |
| Transmission timeout | `ERROR` | Reset with `AT+ABORT` |
| Communication lost | No response | Send `AT` to test connection |

### Parameter Validation

- **Frequency**: Must be between 400.0 and 1000.0 MHz
- **Power**: Must be between 0 and 20 dBm (both devices)
- **Frequency Correction**: -50.0 to +50.0 PPM (v3.6+: 0.02 decimal precision)
- **Capcode**: Valid FLEX capcode (numeric)
- **Binary data length**: 1-2048 bytes
- **FLEX message**: Maximum 248 characters (both TTGO and Heltec)
- **WiFi SSID/Password**: Standard WiFi format
- **API Port**: 1024-65535
- **Banner**: 1-16 characters

## ⚡ Advanced Usage

### Frequency Calibration (All Firmware Versions v1, v2, v3)

All firmware versions include frequency calibration to compensate for crystal oscillator tolerances and temperature drift.

**When to Use**:
- When observed transmission frequency differs from commanded frequency
- To compensate for crystal oscillator accuracy (typically ±20-50 PPM)
- For temperature compensation in varying environments

**Calibration Process**:
1. **Measure Frequency Error**: Use SDR software to observe actual vs. intended frequency
2. **Calculate PPM Error**: `PPM = (observed_freq - intended_freq) / intended_freq * 1,000,000`
3. **Apply Correction**: Use `AT+FREQPPM=<ppm_value>` to set correction
4. **Verify**: Test transmission and adjust if needed

**Example Calibration**:
```bash
# Scenario: 932MHz transmission observed at 932.004MHz (4kHz high)
# PPM Error = (932.004 - 932.000) / 932.000 * 1,000,000 = 4.29 PPM

# Apply negative correction to reduce frequency
AT+FREQPPM=-4.3

# Save configuration (v3 firmware)
AT+SAVE

# Verify correction applied
AT+FREQPPM?
# Response: +FREQPPM: -4.3
```

**Notes**:
- Correction range: -50.0 to +50.0 PPM
- **v3.6+**: Enhanced precision with 0.02 decimal increments (vs. 0.1 previously)
- Applied to all frequency settings (AT commands, web interface, API)
- Available on all firmware versions (v1, v2, v3)
- **v1 and v2 firmware**: Correction stored in RAM only (resets on power cycle)
- **v3 firmware**: Correction automatically saved to SPIFFS with AT+SAVE (persists across power cycles)
- Use AT+SAVE after setting PPM correction in v3 firmware to ensure persistence

### Automated Scripting

```bash
#!/bin/bash
# Configure device and send message

# TTGO LoRa32
echo -e "AT+FREQ=929.6625\r\nAT+POWER=15\r\nAT+MSG=1234567\r\n" | screen /dev/ttyACM0 115200

# Heltec WiFi LoRa32 V2
# echo -e "AT+FREQ=929.6625\r\nAT+POWER=15\r\nAT+MSG=1234567\r\n" | screen /dev/ttyUSB0 115200
```

### Python Integration

```python
import serial
import time

def send_flex_message(port, capcode, message):
    """
    Send FLEX message via AT commands

    Args:
        port: Serial port ('/dev/ttyACM0' for TTGO, '/dev/ttyUSB0' for Heltec)
        capcode: Target capcode (numeric)
        message: Message text (max 248 characters)

    Returns:
        bool: True if successful, False otherwise
    """
    ser = serial.Serial(port, 115200, timeout=5)

    # Send AT+MSG command
    ser.write(f'AT+MSG={capcode}\r\n'.encode())
    response = ser.readline().decode().strip()

    if '+MSG: READY' in response:
        # Send message
        ser.write(f'{message}\r\n'.encode())
        response = ser.readline().decode().strip()
        return 'OK' in response

    return False

# Usage examples
# TTGO LoRa32
success = send_flex_message('/dev/ttyACM0', 1234567, 'Hello World!')

# Heltec WiFi LoRa32 V2
# success = send_flex_message('/dev/ttyUSB0', 1234567, 'Hello World!')

print(f"Message sent: {success}")
```

### REST API Alternative (v3 Firmware)

Instead of AT commands, you can use the REST API:

```bash
# Both devices support REST API in v3 firmware
curl -X POST http://DEVICE_IP/api \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":10,"message":"Hello World"}'
```

## 🔧 Troubleshooting

**🔧 Complete Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive AT command issue resolution covering hardware problems, communication errors, firmware-specific issues, and professional problem reporting.

### Quick AT Command Issues

1. **No response to AT commands**:
   - Verify correct serial port and baud rate (115200)
   - **TTGO LoRa32**: Check `/dev/ttyACM0` (Linux) or `COM3+` (Windows)
   - **Heltec WiFi LoRa32 V2**: Check `/dev/ttyUSB0` (Linux) or `COM4+` (Windows)
   - Check USB cable and connections
   - Try sending a simple `AT` command

2. **Commands return ERROR**:
   - Check command syntax and parameter ranges
   - Ensure device is not busy (check AT+STATUS?)
   - Verify firmware version supports the command

3. **WiFi commands not working** (v3 firmware):
   - Ensure v3 firmware is installed
   - Check WiFi is enabled: `AT+WIFIENABLE?`
   - Verify SSID and password are correct

## 📊 Device Specifications

### TTGO LoRa32

| Specification | Value |
|--------------|-------|
| **MCU** | ESP32 (240MHz dual-core Xtensa LX6) |
| **Radio Chipset** | SX1276 (433/868/915 MHz) |
| **Serial Port** | `/dev/ttyACM0` (Linux), `COM3+` (Windows) |
| **Power Range** | 0 to +20 dBm |
| **Frequency Range** | 400-1000 MHz |
| **Max Message Length** | 248 characters |
| **Default Frequency** | 915.0 MHz |
| **Display** | 128x64 OLED (U8g2 library) |
| **Status** | ✅ Fully supported |

### Heltec WiFi LoRa32 V2

| Specification | Value |
|--------------|-------|
| **MCU** | ESP32 (240MHz dual-core Xtensa LX6) |
| **Radio Chipset** | SX1276 (433/868/915 MHz) |
| **Serial Port** | `/dev/ttyUSB0` (Linux), `COM4+` (Windows) |
| **Power Range** | 0 to +20 dBm |
| **Frequency Range** | 400-1000 MHz |
| **Max Message Length** | 248 characters |
| **Default Frequency** | 929.6625 MHz |
| **Display** | 128x64 OLED (Heltec library) |
| **Status** | ✅ Fully supported |

**Note**: Both devices use the SX1276 chipset and support full 248-character FLEX messages.

## 📚 Related Documentation

- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message
- **[README.md](../README.md)**: Project overview and quick start
- **[REST_API.md](REST_API.md)**: REST API reference for v3 firmware
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface user guide
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware installation guide
- **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)**: Comprehensive troubleshooting guide

## 🤝 Support

For AT command issues:
1. Check device status with `AT+STATUS?`
2. Verify firmware version compatibility
3. Consult the troubleshooting section above
4. Review parameter ranges and syntax
5. Test with simple commands first (AT, AT+STATUS?)
6. Verify correct serial port for your device type
