# FLEX-FSK-TX v2.5

**UART/Serial AT Command Firmware with Enhanced Features**

Clean, modular implementation combining the best of v2 and v3.6 without WiFi/Web/API dependencies.

---

## Features

### Core Functionality
- ✅ **AT Command Protocol** - Full-featured serial communication
- ✅ **FLEX Encoding** - On-device FLEX message encoding (tinyflex)
- ✅ **Core 0 Transmission** - Isolated RF transmission on dedicated core
- ✅ **Message Queue** - 10-message FIFO queue
- ✅ **EMR Support** - Emergency Message Resynchronization

### Configuration & Storage
- ✅ **NVS (Preferences)** - Persistent core config (PPM correction)
- ✅ **SPIFFS** - Application settings in JSON format
- ✅ **Factory Reset** - GPIO 0 button (30s hold) or AT command

### Hardware Support
- ✅ **RF Amplifier Control** - Configurable GPIO, polarity, delay
- ✅ **Battery Monitoring** - Voltage, percentage, charging detection
- ✅ **RTC Support (DS3231)** - Optional real-time clock
- ✅ **OLED Display** - Status display with 5-minute timeout
- ✅ **Heartbeat LED** - 4 blinks every 60 seconds

### Diagnostics & Reliability
- ✅ **Persistent Logging** - SPIFFS log file with RAM buffering
- ✅ **Watchdog Timer** - 60-second timeout with boot protection
- ✅ **Boot Failure Tracking** - Auto-recovery after failures
- ✅ **Message Truncation** - Auto-truncate to 248 chars
- ✅ **Capcode Validation** - FLEX protocol range validation

### Alerts
- ✅ **Low Battery Alert** - Send FLEX message at 10% (configurable)
- ✅ **Power Disconnect Alert** - Detect power loss with hysteresis

---

## Supported Hardware

- **TTGO LoRa32-OLED** (ESP32 + SX1276)
- **Heltec WiFi LoRa 32 V2** (ESP32 + SX1276)

Board selection via `#define` in `config.h` (default: TTGO)

---

## AT Commands

### Basic Commands
```
AT                          - Test AT interface
AT+RESET                    - Reset device
AT+ABORT                    - Abort current operation
AT+STATUS?                  - Query device state
```

### Radio Configuration
```
AT+FREQ=<MHz>               - Set frequency (400-1000 MHz)
AT+FREQ?                    - Query frequency
AT+FREQPPM=<ppm>            - Set PPM correction (-50 to +50)
AT+FREQPPM?                 - Query PPM correction
AT+POWER=<dBm>              - Set TX power (2-20 dBm)
AT+POWER?                   - Query TX power
```

### Message Transmission
```
AT+SEND=<bytes>             - Send binary data
AT+MSG=<capcode>            - Send FLEX message (text follows)
AT+MAILDROP=<0|1>           - Set/query mail drop flag
AT+MAILDROP?
```

### Device Information
```
AT+DEVICE?                  - Comprehensive device info
                              (firmware, battery, memory, FLEX defaults)
```

### FLEX Defaults
```
AT+FLEX?                    - Query all FLEX defaults
AT+FLEX=CAPCODE,<capcode>   - Set default capcode
AT+FLEX=FREQUENCY,<MHz>     - Set default frequency
AT+FLEX=POWER,<dBm>         - Set default power
```

### Logging
```
AT+LOGS?<N>                 - Query last N log lines (default 25)
AT+RMLOG                    - Delete log file
```

### Factory Reset
```
AT+FACTORYRESET             - Factory reset (clear SPIFFS)
```

---

## File Structure

```
flex-fsk-tx-v2.5/
├── flex-fsk-tx-v2.5.ino    # Main firmware file (setup/loop)
├── config.h                 # Configuration constants
│
├── storage.h / .cpp         # NVS + SPIFFS management
├── logging.h / .cpp         # Persistent logging system
├── utils.h / .cpp           # Utility functions
│
├── hardware.h / .cpp        # Hardware abstraction
│                             (Radio, OLED, Battery, RTC, RF Amp)
├── display.h / .cpp         # OLED display logic
│
├── flex_protocol.h / .cpp   # FLEX encoding, EMR, queue
├── transmission.h / .cpp    # Core 0 transmission task
│
├── at_commands.h / .cpp     # AT protocol parser & handlers
│
├── boards/                  # Symlink to ../../include/boards
├── tinyflex/                # Symlink to ../../include/tinyflex
└── README.md                # This file
```

---

## Configuration Files

### NVS (Preferences) - `/config` namespace
```cpp
struct CoreConfig {
    uint32_t magic;
    uint8_t version;
    float frequency_correction_ppm;  // Survives factory reset
};
```

### SPIFFS - `/settings.json`
```json
{
  "banner": "flex-fsk-tx",
  "timezone_offset": 0.0,
  "alerts": {
    "low_battery": true,
    "power_disconnect": true
  },
  "rf_amplifier": {
    "enabled": false,
    "power_pin": 32,
    "delay_ms": 200,
    "active_high": true
  },
  "flex": {
    "frequency": 931.9375,
    "capcode": "37137",
    "txpower": 10.0
  }
}
```

### SPIFFS - `/serial.log`
- 32KB max file size
- 8KB kept on rotation
- 2KB RAM buffer
- Flush every 1 second
- Timestamps (RTC or uptime)

---

## Compilation

### Required Libraries (Arduino Library Manager)
- **RadioLib** by Jan Gromeš
- **U8g2** by oliver
- **ArduinoJson** by Benoit Blanchon
- **RTClib** by Adafruit (if `RTC_ENABLED true`)

### Board Selection
Edit `config.h` line 10-12:
```cpp
#if !defined(TTGO_LORA32_V21) && !defined(HELTEC_WIFI_LORA32_V2)
  #define TTGO_LORA32_V21  // or HELTEC_WIFI_LORA32_V2
#endif
```

### Optional Features
Edit `config.h` line 18:
```cpp
#define RTC_ENABLED true   // Enable DS3231 RTC support
// #define ENABLE_DEBUG    // Uncomment for verbose debug
```

### Arduino IDE
1. Open `flex-fsk-tx-v2.5.ino`
2. Select board: `ESP32 Dev Module` or board-specific
3. Verify/Compile
4. Upload

---

## Default Pin Assignments

### TTGO LoRa32-OLED
- **RF Amplifier**: GPIO 32 (default)
- **Battery ADC**: GPIO 35
- **LED**: GPIO 25
- **Factory Reset**: GPIO 0

### Heltec WiFi LoRa 32 V2
- **RF Amplifier**: GPIO 22 (default)
- **Battery ADC**: GPIO 37
- **LED**: GPIO 25
- **Factory Reset**: GPIO 0

See `include/boards/boards.h` for complete pin definitions.

---

## Usage Example

### Basic Message Transmission
```bash
# Via serial terminal (115200 baud)
AT
AT+FREQ=929.6625
AT+POWER=10
AT+MSG=1234567
Hello World!
```

### Configure Defaults
```bash
AT+FLEX=CAPCODE,1234567
AT+FLEX=FREQUENCY,929.6625
AT+FLEX=POWER,10
```

### Query Device Status
```bash
AT+DEVICE?
+DEVICE_FIRMWARE: v2.5.0
+DEVICE_BATTERY: 85%
+DEVICE_MEMORY: 245632 bytes
+DEVICE_FLEX_CAPCODE: 1234567
+DEVICE_FLEX_FREQUENCY: 929.6625
+DEVICE_FLEX_POWER: 10.0
OK
```

### View Logs
```bash
AT+LOGS?50
2025-04-03 20:30:15 SYSTEM: Boot complete
2025-04-03 20:31:22 QUEUE: Added message (count=1, capcode=1234567)
2025-04-03 20:31:23 TRANSMISSION: Success
...
OK
```

---

## Differences from v2 and v3.6

### From v2
**Added:**
- Core 0 transmission isolation
- Message queue
- EMR synchronization
- Persistent configuration (NVS + SPIFFS)
- Persistent logging
- Battery monitoring + alerts
- RTC support
- RF amplifier control
- Watchdog timer
- Factory reset
- Enhanced AT commands (DEVICE, FLEX, LOGS)
- Message truncation
- Capcode validation

### From v3.6
**Removed:**
- WiFi connection/scanning/AP mode
- Web server (HTTP handlers)
- REST API
- MQTT client
- IMAP email monitoring
- ChatGPT integration
- NTP time sync
- Remote syslog
- Grafana webhook

**Kept:**
- All core improvements (queue, EMR, Core 0, etc.)
- Configuration system (NVS + SPIFFS)
- Logging system (local only)
- Hardware enhancements (battery, RTC, RF amp)
- Better code structure

---

## Architecture

### Multi-Core Operation
- **Core 1 (loop)**: AT command processing, logging, battery monitoring, display
- **Core 0 (task)**: RF transmission with isolated timing

### Thread Safety
- Message queue protected with `portMUX`
- Display updates via flags (no direct calls from Core 0)
- Atomic state transitions

---

## License

GNU General Public License v3.0

---

## Version

**v2.5.0** - Initial modular release combining v2 and v3.6 features

