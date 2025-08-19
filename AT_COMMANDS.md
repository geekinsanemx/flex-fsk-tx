# TTGO FLEX Transmitter - AT Commands Guide

Complete guide for using the AT command interface to control the TTGO FLEX paging transmitter via serial communication.

> **Note**: AT commands are available in all firmware versions (v1, v2, v3). This guide includes WiFi and advanced commands specific to v3 firmware.

## 🔗 Connection Setup

### Hardware Connection
- **Interface**: USB Serial (Virtual COM Port)
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

### Software Options

#### Arduino IDE Serial Monitor
1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Set line ending to **Both NL & CR**
4. Type commands in the input field

#### PuTTY (Windows)
```
Connection Type: Serial
Serial Line: COM3 (adjust for your port)
Speed: 115200
Data bits: 8
Stop bits: 1
Parity: None
Flow control: None
```

#### Screen (Linux/macOS)
```bash
screen /dev/ttyUSB0 115200
# or
screen /dev/ttyACM0 115200
```

#### Minicom (Linux)
```bash
minicom -b 115200 -D /dev/ttyUSB0
```

#### Python Terminal
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
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
| `AT+POWER=<value>` | Set | `<value>`: -9 to 20 (dBm) | `OK` / `ERROR` | v1,v2,v3 | Set transmission power |
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
| `AT+SAVE` | Execute | None | `OK` / `ERROR` | v3 | Save configuration to EEPROM |
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

## 📡 Command Usage Examples

### Basic Operation

```bash
# Test connection
AT

# Check device status
AT+STATUS?

# Set frequency to 931.9375 MHz
AT+FREQ=931.9375

# Set transmit power to 10 dBm
AT+POWER=10

# Query current settings
AT+FREQ?
AT+POWER?
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

# Save all settings to EEPROM
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
- **Power**: Must be between -9 and 20 dBm
- **Capcode**: Valid FLEX capcode (numeric)
- **Binary data length**: 1-2048 bytes
- **FLEX message**: Maximum 240 characters
- **WiFi SSID/Password**: Standard WiFi format
- **API Port**: 1024-65535
- **Banner**: 1-16 characters

## ⚡ Advanced Usage

### Automated Scripting

```bash
#!/bin/bash
# Configure device and send message
echo -e "AT+FREQ=929.6625\r\nAT+POWER=15\r\nAT+MSG=1234567\r\n" | screen /dev/ttyACM0 115200
```

### Python Integration

```python
import serial
import time

def send_flex_message(port, capcode, message):
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

# Usage
success = send_flex_message('/dev/ttyACM0', 1234567, 'Hello World!')
print(f"Message sent: {success}")
```

### REST API Alternative (v3 Firmware)

Instead of AT commands, you can use the REST API:

```bash
curl -X POST http://DEVICE_IP:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":10,"message":"Hello World"}'
```

## 🔧 Troubleshooting

### Connection Issues

1. **No response to AT commands**:
   - Verify correct serial port and baud rate (115200)
   - Check USB cable and connections
   - Ensure device is powered on
   - Try sending a simple `AT` command

2. **Garbled output**:
   - Confirm baud rate is 115200
   - Check line ending settings (CR+LF)
   - Verify no other applications are using the serial port

3. **Commands return ERROR**:
   - Check command syntax and parameter ranges
   - Ensure device is not busy (check AT+STATUS?)
   - Verify firmware version supports the command

### v3 Firmware Specific Issues

1. **WiFi commands not working**:
   - Ensure v3 firmware is installed
   - Check WiFi is enabled: `AT+WIFIENABLE?`
   - Verify SSID and password are correct

2. **REST API not accessible**:
   - Check device is connected to WiFi: `AT+WIFI?`
   - Verify API port: `AT+APIPORT?`
   - Test with correct credentials: `AT+APIUSER?`

## 📚 Related Documentation

- **[README.md](README.md)**: Project overview and quick start
- **[API.md](API.md)**: REST API reference for v3 firmware
- **[USER.md](USER.md)**: Web interface user guide
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware installation guide

## 🤝 Support

For AT command issues:
1. Check device status with `AT+STATUS?`
2. Verify firmware version compatibility
3. Consult the troubleshooting section above
4. Review parameter ranges and syntax
5. Test with simple commands first (AT, AT+STATUS?)