# FLEX Paging Message Transmitter - AT Commands Guide

Complete guide for using the AT command interface to control the FLEX paging message transmitter via serial communication.

> **Note**: this is a single-variant firmware (all commands below are present in every build; GSM
> support itself is a compile-time toggle in `config.h`, unrelated to which AT commands exist).

## Connection Setup

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

Alternatively, use the optional [host CLI](../host/README.md) which wraps this interface.

## Complete AT Commands Reference

This is the complete command set parsed by `at_commands.cpp`'s `at_parse_command()`. There is no
tiered/versioned subset — every command below is available on every build of this firmware.

### Basic Commands

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT` | Test | None | `OK` | Test communication and reset device state |
| `AT+STATUS?` | Query | None | `+STATUS: <state>`<br>`OK` | Query current device status |
| `AT+ABORT` | Execute | None | `OK` | Abort current operation |
| `AT+RESET` | Execute | None | `OK` (then restart) | Software reset device |

### Radio Configuration Commands

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT+FREQ=<value>` | Set | `<value>`: 400.0-1000.0 (MHz) | `OK` / `ERROR` | Set transmission frequency |
| `AT+FREQ?` | Query | None | `+FREQ: <value>`<br>`OK` | Query current frequency setting |
| `AT+FREQPPM=<value>` | Set | `<value>`: -50.0 to +50.0 (PPM) | `OK` / `ERROR` | Set frequency correction in PPM |
| `AT+FREQPPM?` | Query | None | `+FREQPPM: <value>`<br>`OK` | Query current frequency correction |
| `AT+POWER=<value>` | Set | `<value>`: -9 to 20 (dBm) | `OK` / `ERROR` | Set transmission power |
| `AT+POWER?` | Query | None | `+POWER: <value>`<br>`OK` | Query current power setting |

### Message Transmission Commands

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT+SEND=<length>` | Execute | `<length>`: 1-2048 (bytes) | `+SEND: READY` | Initiate binary FLEX-frame transmission (host encodes locally, e.g. via the [host CLI](../host/README.md)/tinyflex) |
| `AT+MSG=<capcode>` | Execute | `<capcode>`: target capcode | `+MSG: READY`, then send the text and press Enter | Send FLEX message with on-device encoding |

There is no AT command for the FLEX mail-drop bit — it is set purely by the local encoder used
ahead of `AT+SEND` (see the host CLI's `-m/--maildrop`). `AT+MSG` has no mail-drop equivalent.

### Network Commands

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT+NETWORK?` | Query | None | `+NETWORK: <mode>` | Query current network transport mode |
| `AT+NETWORK=<mode>` | Set | `AUTO`, `WIFI`, `GSM`, `AP` | `OK` / `ERROR` | Lock network transport mode (resets to `AUTO` on reboot) |
| `AT+WIFI?` | Query | None | `+WIFI: <status>[,<ip>]` | Query WiFi connection status |
| `AT+WIFI=<ssid>,<password>` | Set | SSID (1-32 chars), password (0-64 chars) | `OK` / `ERROR` | Add/update a stored WiFi network |

### Default FLEX Settings Commands

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT+FLEX?` | Query | None | `+FLEX_CAPCODE: <v>`<br>`+FLEX_FREQUENCY: <v>`<br>`+FLEX_POWER: <v>`<br>`OK` | Query default FLEX settings |
| `AT+FLEX=CAPCODE,<value>` | Set | Capcode > 0 | `OK` / `ERROR` | Set default capcode |
| `AT+FLEX=FREQUENCY,<value>` | Set | 400.0-1000.0 (MHz) | `OK` / `ERROR` | Set default frequency |
| `AT+FLEX=POWER,<value>` | Set | 0.0-20.0 (dBm) | `OK` / `ERROR` | Set default power |

Every `AT+FLEX=...`/`AT+NETWORK=...`/`AT+WIFI=...` set auto-saves via `save_runtime_settings()` —
there is no separate save command. Note the default-power range for `AT+FLEX=POWER,...`
(0.0-20.0) differs from the immediate `AT+POWER=` range (-9 to 20) — this reflects the firmware's
actual validation, not a documentation inconsistency.

### Device Status Commands

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT+DEVICE?` | Query | None | Multi-line status (below) | Full device status dump |
| `AT+FACTORYRESET` | Execute | None | `OK` (then restart) | Reset all settings to factory defaults |

`AT+DEVICE?` returns, one per line before the final `OK`:
```
+DEVICE_FIRMWARE: <version>
+DEVICE_BATTERY: <percent>% | N/A
+DEVICE_WIFI: Connected | Disconnected | AP_Mode
+DEVICE_MQTT: Connected | Disconnected | Disabled
+DEVICE_IMAP: Active | Disabled
+DEVICE_IMAP_ACCOUNT<N>: Active | Suspended   (repeated per configured account)
+DEVICE_API: Enabled | Disabled
+DEVICE_GRAFANA: Enabled | Disabled
+DEVICE_MEMORY: <bytes> bytes
+DEVICE_FLEX_CAPCODE: <value>
+DEVICE_FLEX_FREQUENCY: <value>
+DEVICE_FLEX_POWER: <value>
OK
```

### Log & Diagnostics Commands

| Command | Type | Parameters | Response | Description |
|---------|------|------------|----------|-------------|
| `AT+LOGS?` | Query | None | Last 25 log lines + `OK` | Query last 25 lines of persistent log |
| `AT+LOGS?N` | Query | `N`: number of lines | Last N log lines + `OK` | Query last N lines of persistent log |
| `AT+RMLOG` | Execute | None | `LOG: File deleted` + `OK` | Delete persistent log file |

**Any multi-line response (`AT+FLEX?`, `AT+DEVICE?`) must be read as all `+`-prefixed lines up to
the terminating `OK`/`ERROR`** — clients that only keep the last line will silently drop data.

## Device Status States

| Status | Description |
|--------|-------------|
| `READY` | Device idle and ready for commands |
| `WAITING_DATA` | Device waiting for binary data after `AT+SEND` |
| `WAITING_MSG` | Device waiting for text message after `AT+MSG` |
| `TRANSMITTING` | Device currently transmitting data |
| `ERROR` | Device in error state |
| `WIFI_CONNECTING` | WiFi connection in progress |
| `WIFI_AP_MODE` | Access Point mode active |
| `NTP_SYNC` | NTP time sync in progress |
| `MQTT_CONNECTING` | MQTT connection in progress |

While any of these AT commands are being processed with a transmission in progress, only `AT`,
`AT+STATUS?`, and `AT+ABORT` are accepted — everything else returns `ERROR` until the transmission
guard clears.

**Note**: the web interface and REST API share a message queue that can hold up to 25 messages,
processed sequentially, reducing "device busy" scenarios seen over AT commands during heavy use.

## Command Usage Examples

### Basic Operation

```bash
# Test connection
AT

# Check device status
AT+STATUS?

# Set frequency (unified default is 931.9375 MHz)
AT+FREQ=931.9375

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

### Binary Data Transmission

```bash
# Send 10 bytes of binary data
AT+SEND=10
# Wait for "+SEND: READY" response
# Then send exactly 10 bytes of binary data
# Device responds with "OK" when complete
```

### FLEX Message Transmission (on-device encoding)

```bash
# Send FLEX message to capcode 1234567
AT+MSG=1234567
# Wait for "+MSG: READY" response
# Type your message and press Enter:
Hello World!
# Device responds with "OK" when transmitted
```

### Network Configuration

```bash
# Configure/add a WiFi network
AT+WIFI=MyNetwork,MyPassword

# Check WiFi status
AT+WIFI?
# Response: +WIFI: CONNECTED,192.168.1.100

# Query current network transport mode
AT+NETWORK?
# Response: +NETWORK: AUTO

# Lock to WiFi only (disables automatic failover)
AT+NETWORK=WIFI

# Lock to GSM only
AT+NETWORK=GSM

# Force AP mode
AT+NETWORK=AP

# Return to automatic failover
AT+NETWORK=AUTO
```

**Network Mode Behavior**:
- **AUTO**: Default. Automatic WiFi → GSM → AP failover
- **WIFI**: WiFi only, retries every 60 seconds if disconnected
- **GSM**: GSM only, retries every 300 seconds (5 minutes) if disconnected
- **AP**: Access Point mode, no retries
- **Display**: Shows asterisk when locked (e.g., `WiFi*`, `GSM*`, `AP*`)
- **Reset**: Mode resets to `AUTO` on reboot or manual mode change

### Default FLEX Settings

```bash
# Query all default FLEX settings
AT+FLEX?
# Response:
# +FLEX_CAPCODE: 1234567
# +FLEX_FREQUENCY: 931.9375
# +FLEX_POWER: 2.0
# OK

# Set default capcode
AT+FLEX=CAPCODE,1234567

# Set default frequency
AT+FLEX=FREQUENCY,931.9375

# Set default power
AT+FLEX=POWER,10
```

### Device Status and Factory Reset

```bash
# Full status dump
AT+DEVICE?

# Factory reset (restores all defaults, restarts device)
AT+FACTORYRESET
```

### Persistent Log Commands

```bash
# Query last 25 lines of log (default)
AT+LOGS?
# Response: timestamped log lines (oldest→newest)
# OK

# Query last 50 lines
AT+LOGS?50

# Query last 10 lines
AT+LOGS?10

# Delete log file
AT+RMLOG
# Response: LOG: File deleted
# OK

# If no log file exists:
AT+LOGS?
# Response: ERROR: No log file found
# OK
```

**Log File Details**:
- **File**: `/serial.log` on SPIFFS
- **Max Size**: 64KB (auto-truncates to last 32KB)
- **Timestamp Format (pre-NTP/RTC)**: `0000-00-00 HH:MM:SS` (uptime-based)
- **Timestamp Format (post-NTP/RTC)**: `YYYY-MM-DD HH:MM:SS`
- **Order**: Chronological (oldest → newest)

## Error Handling

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
| Device busy (transmission in progress) | `ERROR` | Wait and retry, or send `AT+STATUS?` |
| Transmission timeout | `ERROR` | Reset with `AT+ABORT` |
| Communication lost | No response | Send `AT` to test connection |

### Parameter Validation

- **Frequency**: 400.0 to 1000.0 MHz (`AT+FREQ`, `AT+FLEX=FREQUENCY,...`)
- **Power**: -9 to 20 dBm (`AT+POWER`); 0.0 to 20.0 dBm (`AT+FLEX=POWER,...`)
- **Frequency correction**: -50.0 to +50.0 PPM
- **Capcode**: numeric, > 0
- **Binary data length**: 1-2048 bytes
- **FLEX message**: maximum 248 characters (both TTGO and Heltec)
- **WiFi SSID**: 1-32 characters; **password**: 0-64 characters

## Advanced Usage

### Frequency Calibration

The firmware includes frequency calibration to compensate for crystal oscillator tolerances and temperature drift.

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

# Verify correction applied
AT+FREQPPM?
# Response: +FREQPPM: -4.3
```

**Notes**:
- Correction range: -50.0 to +50.0 PPM
- Applied to all frequency settings (AT commands, web interface, API)
- `AT+FREQPPM=` auto-saves via `save_runtime_settings()` — no separate save step needed, and the
  correction persists across power cycles

### Automated Scripting

```bash
#!/bin/bash
# Configure device and send message

# TTGO LoRa32
echo -e "AT+FREQ=931.9375\r\nAT+POWER=15\r\nAT+MSG=1234567\r\n" | screen /dev/ttyACM0 115200

# Heltec WiFi LoRa32 V2
# echo -e "AT+FREQ=931.9375\r\nAT+POWER=15\r\nAT+MSG=1234567\r\n" | screen /dev/ttyUSB0 115200
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

### REST API Alternative

Instead of AT commands, you can use the REST API:

```bash
curl -X POST http://DEVICE_IP/api \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":931.9375,"power":10,"message":"Hello World"}'
```

## Troubleshooting

**Complete Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive AT command issue resolution covering hardware problems, communication errors, and problem reporting.

### Quick AT Command Issues

1. **No response to AT commands**:
   - Verify correct serial port and baud rate (115200)
   - **TTGO LoRa32**: Check `/dev/ttyACM0` (Linux) or `COM3+` (Windows)
   - **Heltec WiFi LoRa32 V2**: Check `/dev/ttyUSB0` (Linux) or `COM4+` (Windows)
   - Check USB cable and connections
   - Try sending a simple `AT` command

2. **Commands return ERROR**:
   - Check command syntax and parameter ranges
   - Ensure device is not busy (check `AT+STATUS?`)
   - Confirm the command exists in this firmware (see the reference table above — do not assume
     commands from other FLEX firmware projects apply here)

3. **WiFi commands not working**:
   - Check current status: `AT+WIFI?`
   - Verify SSID and password are correct
   - Check the current network mode isn't locked away from WiFi: `AT+NETWORK?`

## Device Specifications

### TTGO LoRa32

| Specification | Value |
|--------------|-------|
| **MCU** | ESP32 (240MHz dual-core Xtensa LX6) |
| **Radio Chipset** | SX1276 (433/868/915 MHz) |
| **Serial Port** | `/dev/ttyACM0` (Linux), `COM3+` (Windows) |
| **Power Range** | -9 to +20 dBm |
| **Frequency Range** | 400-1000 MHz |
| **Max Message Length** | 248 characters |
| **Default Frequency** | 931.9375 MHz |
| **Display** | 128x64 OLED (U8g2 library) |
| **Status** | Fully supported |

### Heltec WiFi LoRa32 V2

| Specification | Value |
|--------------|-------|
| **MCU** | ESP32 (240MHz dual-core Xtensa LX6) |
| **Radio Chipset** | SX1276 (433/868/915 MHz) |
| **Serial Port** | `/dev/ttyUSB0` (Linux), `COM4+` (Windows) |
| **Power Range** | -9 to +20 dBm |
| **Frequency Range** | 400-1000 MHz |
| **Max Message Length** | 248 characters |
| **Default Frequency** | 931.9375 MHz |
| **Display** | 128x64 OLED (Heltec library) |
| **Status** | Fully supported |

**Note**: both devices use the SX1276 chipset, share the same default frequency, and support full
248-character FLEX messages.

## Related Documentation

- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message
- **[README.md](../README.md)**: Project overview and quick start
- **[REST_API.md](REST_API.md)**: REST API reference
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface user guide
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware installation guide
- **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)**: Comprehensive troubleshooting guide
- **[../host/README.md](../host/README.md)**: Optional PC-side CLI companion

## Support

For AT command issues:
1. Check device status with `AT+STATUS?`
2. Consult the troubleshooting section above
3. Review parameter ranges and syntax against the reference table above
4. Test with simple commands first (`AT`, `AT+STATUS?`)
5. Verify correct serial port for your device type
