# FLEX Paging Message Transmitter - User Guide

Complete user guide for operating the FLEX paging message transmitter. This guide covers the user-friendly web interface, basic AT command usage, and REST API overview.

> **Hardware Requirement**: This guide is for **ESP32 LoRa32 devices** (TTGO LoRa32-OLED or Heltec WiFi LoRa 32 V2) running this single-variant firmware. The web interface, REST API, and WiFi features are present on every build — there is no separate AT-only build.

## 🚀 Getting Started

### What You'll Need

**Hardware**:
- ESP32 LoRa32 development board:
  - TTGO LoRa32-OLED (ESP32 + SX1276 radio) - Fully supported
  - Heltec WiFi LoRa 32 V2 (ESP32 + SX1276 radio) - Fully supported
  - Heltec WiFi LoRa 32 **V3** (SX1262 radio) is **NOT supported** — the SX1262 chipset is
    incompatible with this firmware's FLEX protocol timing requirements
- USB cable for initial setup
- Appropriate antenna for your frequency band

**Firmware**:
- This firmware must be installed on your ESP32 device (running version is reported by `AT+DEVICE?`'s `+DEVICE_FIRMWARE` line, sourced from `version.h`)
- If not installed, see [FIRMWARE.md](FIRMWARE.md) for complete flashing instructions

**Network**:
- WiFi network with internet access (for web interface)
- Computer, smartphone, or tablet with web browser

### First Time Setup Overview

1. **Power On**: Connect ESP32 device via USB
2. **Initial Configuration**: Connect to device's WiFi hotspot
3. **Network Setup**: Configure your WiFi network
4. **Ready to Use**: Access web interface for message transmission

## 📶 Initial WiFi Setup (First Time)

When you power on your ESP32 device for the first time, it will create its own WiFi
hotspot for configuration. This is the same on both TTGO and Heltec V2 — it's part of the
single firmware build, not a separate WiFi-enabled variant.

### Step 1: Connect to Device Hotspot

1. **Power on** your ESP32 device (via USB or battery)
2. **Wait 30 seconds** for the device to boot completely
3. **Check the OLED display** - it should show "flex-fsk-tx" banner, then the AP SSID and
   password
4. **Look for the WiFi network** shown on the display: `FLEX_XXXX` (where XXXX is 4 hex
   characters derived from the device's MAC address, e.g., `FLEX_A1B2`)

**WiFi Network Details**:
- **Network Name**: `FLEX_XXXX` (unique per device, shown on OLED)
- **Password**: MAC-derived 8-character password (shown on OLED)
- **Security**: WPA2

### Step 2: Access Configuration Portal

1. **Connect** your computer/phone to the device's WiFi network (`FLEX_XXXX`)
2. **Open web browser** and navigate to:
   ```
   http://192.168.4.1
   ```
3. **Wait for page to load** - you'll see the FLEX Paging Message Transmitter configuration page

### Step 3: Configure Your WiFi Network

1. **Navigate to Configuration**: Click on "Configuration" or go to `http://192.168.4.1/config`

2. **WiFi Network Settings**:
   - **SSID**: Enter your home/office WiFi network name
   - **Password**: Enter your WiFi password
   - **DHCP**: Leave enabled (recommended) or configure static IP if needed

3. **Device Settings** (Optional):
   - **Device Banner**: Custom name displayed on OLED (max 16 characters)

   REST API credentials (username/password) are configured separately, on the
   `/api_config` page — not on this page.

4. **Save Configuration**: Click "Save Settings" button

### Step 4: Connection and Verification

1. **Device Restart**: The ESP32 device will automatically restart and connect to your WiFi
2. **Check OLED Display**: Should show "Connected" and IP address
3. **Note the IP Address**: Write down the IP shown on the display (e.g., `192.168.1.100`)
4. **Test Connection**: Open browser and go to `http://[DEVICE_IP]` (e.g., `http://192.168.1.100`)

### Troubleshooting Initial Setup

**Device hotspot not visible**:
- Wait longer (up to 2 minutes) for device to boot
- Power cycle the device

**Can't connect to 192.168.4.1**:
- Ensure you're connected to the device's WiFi network (`FLEX_XXXX`, shown on OLED)
- Try different browser or clear browser cache
- Check that your device isn't using cellular data

**WiFi connection fails**:
- Verify SSID and password are correct
- Check WiFi network security (WPA2 supported)
- Ensure network allows new devices to connect

## 🌐 Web Interface Usage

Once your ESP32 device is connected to your WiFi network, you can access the web interface from any device on the same network.

### Accessing the Web Interface

1. **Find Device IP**: Check the OLED display or your router's connected devices
2. **Open Browser**: Use any modern web browser
3. **Navigate to Device**: Enter `http://[DEVICE_IP]` (e.g., `http://192.168.1.100`)

### Main Interface - Message Transmission

The main page (`/`) is where you'll send most of your FLEX messages.

**Message Form Fields**:

1. **Capcode** (Required):
   - Target pager capcode (7-10 digits)
   - Valid ranges: 1-1933312, 1998849-2031614, 2101249-4297068542
   - Examples: `1234567`, `8901234567`

2. **Message** (Required):
   - Text message to send (up to 248 characters - auto-truncated if longer)
   - Character counter shows current length with truncation warnings
   - **Truncation Behavior**: Messages longer than 248 characters are auto-truncated to 245 chars + "..."
   - **Visual Indicators**: Counter turns orange at 245+ characters, red when truncation occurs
   - Only printable ASCII characters supported

3. **Frequency** (Required):
   - Transmission frequency in MHz
   - Range: 400.0 to 1000.0 MHz
   - Common examples:
     - `931.9375` MHz (firmware default — `TX_FREQ_DEFAULT` in `config.h`)
     - `929.6625` MHz (US common)
     - `915.0000` MHz (ISM band)

4. **Power** (Required):
   - Transmit power in dBm
   - Range: 0 to 20 dBm
   - Recommended: 10 dBm for most applications

5. **Mail Drop** (Optional):
   - Check box to enable mail drop flag
   - Used for urgent/priority messages
   - Indicates message should be stored if pager is off

**Sending a Message**:
1. Fill in all required fields
2. Review settings (frequency, power, capcode)
3. Click "Send Message" button
4. Wait for confirmation message
5. Check OLED display for transmission status

**Real-Time Validation**:
- Character counter updates as you type
- Invalid values are highlighted in red
- Form won't submit with invalid data

### Configuration Page

Access via `/config` or click "Configuration" link.

**Network Settings**:
- **Current Network**: Shows connected SSID and IP
- **Change Network**: Enter new SSID/password
- **Network Status**: Connection status and signal strength
- **Static IP Configuration**: Optional static IP, gateway, subnet mask, DNS settings

**FLEX Settings**:
- **Default Frequency**: Default transmission frequency in MHz
- **Default TX Power**: Default transmission power in dBm
- **Default Capcode**: Default capcode for message transmission
- **PPM Correction**: Frequency calibration in parts per million (-50.0 to +50.0, 0.02 precision)
  - Used to compensate for crystal oscillator inaccuracy
  - Automatically applied to all frequency settings
  - Example: Set to 4.30 to correct 4kHz offset at 932MHz

**Device Settings**:
- **Banner Message**: Custom text for OLED display (16 chars max)

REST API authentication (username/password) is configured on the separate `/api_config`
page, not here (web-only, no AT command equivalent).

**IMAP Email-to-Pager**:
- **Enable/Disable**: Toggle IMAP email monitoring
- **Server Configuration**: IMAP server, port, SSL/TLS settings
- **Credentials**: Email address and password/app-specific password
- **Check Interval**: How often to check for new emails (minutes)
- **Auto-Send**: Automatically transmit emails as pager messages
- **Format**: Subject line becomes pager message (up to 248 characters)

**MQTT Bidirectional Messaging**:
- **Enable/Disable**: Toggle MQTT connectivity
- **Broker Configuration**: MQTT broker address, port
- **Authentication**: Username and password for MQTT broker
- **TLS/SSL**: Certificate upload for secure connections
- **Topics**: Subscribe/publish topics for message exchange
- **QoS Settings**: Quality of Service level (0, 1, or 2)
- **Persistent Session**: Reliable message delivery when device offline

**ChatGPT Scheduled Prompts**:
- **Enable/Disable**: Toggle ChatGPT integration
- **API Key**: OpenAI API key configuration
- **Scheduled Prompts**: Up to 10 customizable scheduled prompts
- **Prompt Configuration**: Prompt text, schedule (time/days), enable/disable
- **Auto-Send**: Automatically transmit ChatGPT responses as pager messages
- **Status Display**: Shows next execution time and prompt status

**Grafana Webhook Integration**:
- **Enable/Disable**: Toggle Grafana webhook receiver
- **Webhook Endpoint**: URL endpoint for Grafana alerts
- **Authentication**: Optional webhook authentication token
- **Alert Formatting**: Automatic conversion of Grafana alerts to pager messages
- **Priority Mapping**: Map Grafana alert levels to pager priority

**Remote Syslog Logging**:
- **Enable/Disable**: Toggle remote syslog
- **Server Configuration**: Syslog server address, port
- **Transport**: UDP or TCP transport protocol
- **Severity Filter**: Log level filtering (0-7: Emergency to Debug)
- **Facility**: Uses local0 (16) facility code
- **Hostname**: Uses device banner as hostname identifier
- **RFC 3164 Format**: Standard syslog message format

**System Settings**:
- **Factory Reset**: Reset all settings to defaults (clears NVS and SPIFFS)
- **Restart Device**: Reboot the device
- **Save Configuration**: Save current settings to NVS
- **Backup/Restore**: Download/upload complete device configuration as JSON

**System Alerts**:
- **Low Battery Alert**: Alert when battery drops below 10% (with hysteresis)
- **Power Disconnect Alert**: Alert when device switches from power to battery

### Status Page

Access via `/status` or click "Status" link.

**System Status Card**:
- **Device Information**:
  - Firmware version (from `version.h`)
  - Chip model and MAC address
  - Uptime (human-readable: "X days, Y hours, Z mins")
  - Free heap memory (bytes and percentage)

- **Battery Status** (if battery connected):
  - Voltage and percentage (3.2V-4.15V range)
  - Power status (Connected/On Battery)
  - Charging status (Yes/No)
  - Hysteresis protection (4.08V/4.12V thresholds)

- **Network Status**:
  - WiFi connection state and signal strength
  - IP address and gateway
  - DNS servers

- **Time Synchronization**:
  - NTP sync status and last update
  - Current time (local timezone)
  - Configured timezone

- **MQTT Status** (if enabled):
  - Connection state
  - Broker address and port
  - Messages sent/received
  - Persistent session status

- **IMAP Status** (if enabled):
  - Email monitoring state
  - Last check time
  - Messages processed
  - Next check interval

- **Remote Logging** (if enabled):
  - Syslog server connection status
  - Transport protocol (UDP/TCP)
  - Messages sent
  - Severity filter level

**Persistent Device Logs**:
- Reads from persistent SPIFFS log file (`/serial.log`)
- Displays configurable number of log lines (10-500, default 100)
- Chronological order (oldest → newest) with auto-scroll to bottom
- **Live Logs Toggle**: Enable/disable automatic log polling
- **Refresh Interval**: Configurable 1-60 seconds (default 5s), applies immediately
- **Lines Count**: Adjustable 10-500 lines, refreshes display immediately on change
- **Download Button**: Download full log file for offline analysis
- Scrollable container (500px max-height)
- Timestamps: `YYYY-MM-DD HH:MM:SS` (or `0000-00-00 HH:MM:SS` before time sync)

**Device Management**:
- **Factory Reset**: Reset device to defaults
- **Reboot**: Restart the device
- **Download Backup**: Export configuration as JSON

### Web Interface Tips

**Browser Compatibility**:
- Works with any modern browser (Chrome, Firefox, Safari, Edge)
- Mobile browsers fully supported
- No JavaScript required for basic functionality

**Bookmarking**:
- Bookmark `http://[DEVICE_IP]` for quick access
- Create home screen shortcut on mobile devices

**Multiple Users**:
- Multiple people can access the interface simultaneously
- Up to 25 messages can be queued for automatic sequential transmission
- No more "device busy" errors - messages are automatically queued

**Offline Usage**:
- Web interface works without internet connection
- Only requires local network connectivity to device
- IMAP, MQTT, ChatGPT, and Grafana features require internet

## 🔌 Serial Interface Overview (AT Commands)

The ESP32 device also supports direct serial communication via AT commands for advanced users and automation.

### Basic AT Command Usage

**Connection Setup**:
- **Port**: USB serial port:
  - TTGO: typically `/dev/ttyACM0` (Linux), `COM3+` (Windows)
  - Heltec V2: typically `/dev/ttyUSB0` (Linux), `COM4+` (Windows)
- **Baud Rate**: 115200
- **Settings**: 8N1 (8 data bits, no parity, 1 stop bit)

**Common Commands**:
```bash
AT                    # Test connection
AT+FREQ=929.6625      # Set frequency
AT+POWER=10           # Set power
AT+MSG=1234567        # Send message to capcode
Hello World!          # Type message and press Enter
AT+WIFI?              # Check WiFi status
AT+STATUS?            # Check device status
AT+FREQPPM=4.30       # Set PPM correction (0.02 precision)
AT+LOGS?              # View last 25 log lines
AT+LOGS?50            # View last 50 log lines
AT+RMLOG              # Delete log file
```

**When to Use AT Commands**:
- Automation and scripting
- Integration with existing systems
- Batch message processing
- Advanced configuration
- Troubleshooting and diagnostics

**AT Command Benefits**:
- No network required (direct USB connection)
- Faster for repeated operations
- Full device control and configuration
- Real-time status monitoring

> **📚 Complete Reference**: For detailed AT command syntax, parameters, and examples, see [AT_COMMANDS.md](AT_COMMANDS.md)

## 🔧 REST API Overview

For developers and system integrators, the ESP32 device provides a comprehensive REST API for programmatic control.

### Basic REST API Usage

**API Endpoint**:
```
http://[DEVICE_IP]/api
```

**Authentication**:
- HTTP Basic Authentication
- Factory-default credentials: `admin` / `passw0rd` (change immediately)
- Configurable via the web interface's `/api_config` page only — no AT command equivalent
- **Security Warning**: Default credentials display warning banner

**Simple Message Example**:
```bash
curl -X POST http://192.168.1.100/api \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929.6625,
    "power": 10,
    "message": "Hello from API"
  }'
```

### REST API Use Cases

**Home Automation**:
- Integration with smart home systems
- Automated notifications and alerts
- Sensor-triggered messages

**Business Applications**:
- Staff notification systems
- Emergency alert broadcasting
- Automated status updates

**Development and Testing**:
- Automated testing frameworks
- Continuous integration workflows
- Load testing and validation

**System Integration**:
- Legacy pager system replacement
- Integration with existing notification systems
- Custom application development

**IMAP Email-to-Pager**:
- Monitor email inbox for specific messages
- Automatically convert emails to pager messages
- Subject line extraction with 248 character limit

**MQTT Bidirectional Messaging**:
- Publish/subscribe to MQTT topics
- Receive messages from MQTT broker and transmit via pager
- Send pager transmission confirmations back to MQTT

**ChatGPT Scheduled Prompts**:
- Schedule recurring ChatGPT queries
- Automatically send responses to pagers
- Daily summaries, weather updates, reminders

**Grafana Webhook Alerts**:
- Receive Grafana webhook alerts
- Convert alerts to pager messages
- Priority-based alert routing

### API Features

**JSON Format**: Easy to use with any programming language
**Parameter Validation**: Server-side validation with detailed error messages
**Multiple Formats**: Supports both MHz and Hz frequency formats
**Error Handling**: Comprehensive HTTP status codes and error responses
**Rate Limiting**: Message queue prevents device overload (up to 25 messages)
**Message Truncation**: Auto-truncates messages exceeding 248 characters with truncation flag in response

> **🔧 Complete Reference**: For detailed API documentation, programming examples, and integration guides, see [REST_API.md](REST_API.md)

## 📊 Interface Comparison

| Feature | Web Interface | AT Commands | REST API |
|---------|---------------|-------------|----------|
| **Ease of Use** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| **Setup Required** | None | Serial connection | Network setup |
| **Best For** | Manual operation | Automation/scripting | System integration |
| **Network Required** | Yes | No | Yes |
| **Multiple Users** | Yes | No | Yes |
| **Real-time Status** | Yes | Yes | No |
| **Message Queue** | Yes (25 messages) | No | Yes (25 messages) |
| **Batch Operations** | No | Yes | Yes |
| **IMAP Integration** | Yes | Status only | No |
| **MQTT Integration** | Yes | Status only | No |
| **ChatGPT Integration** | Yes | Status only | No |
| **Grafana Webhooks** | Yes | Status only | Yes (`/api/v1/alerts`) |
| **Remote Syslog** | Yes | Status only | No |

"Status only" means `AT+DEVICE?` reports whether the integration is enabled, but AT commands
have no way to configure or trigger it — that's web-interface-only (IMAP/MQTT/ChatGPT/Syslog)
or web/REST-only (Grafana).

## 🎯 Common Usage Scenarios

### Scenario 1: Home User - Manual Messages

**Setup**: Use web interface after initial WiFi configuration
**Typical Use**: Send occasional messages to family pagers
**Recommended Interface**: Web interface

### Scenario 2: Business - Regular Notifications

**Setup**: Configure REST API with custom credentials
**Typical Use**: Automated staff notifications, alerts
**Recommended Interface**: REST API with automation scripts

### Scenario 3: Ham Radio - Technical Operation

**Setup**: Direct serial connection for full control
**Typical Use**: Frequency testing, technical experiments
**Recommended Interface**: AT Commands

### Scenario 4: System Integration

**Setup**: Network configuration with API authentication
**Typical Use**: Integration with existing business systems
**Recommended Interface**: REST API

### Scenario 5: Email-to-Pager Gateway

**Setup**: Configure IMAP settings with email credentials
**Typical Use**: Receive critical emails on pager automatically
**Recommended Interface**: Web interface for IMAP configuration

### Scenario 6: IoT Message Hub

**Setup**: Configure MQTT broker connection
**Typical Use**: Receive messages from IoT devices and sensors
**Recommended Interface**: Web interface for MQTT configuration + REST API for direct messages

### Scenario 7: AI-Powered Notifications

**Setup**: Configure ChatGPT API key and scheduled prompts
**Typical Use**: Daily summaries, weather updates, reminders
**Recommended Interface**: Web interface for prompt scheduling

### Scenario 8: Monitoring & Alerting

**Setup**: Configure Grafana webhook endpoint
**Typical Use**: Receive infrastructure alerts on pager
**Recommended Interface**: Grafana webhook + REST API fallback

## 🔧 Maintenance and Updates

### Regular Maintenance

**Weekly**:
- Check OLED display for any error messages
- Verify web interface accessibility
- Test message transmission
- Review IMAP/MQTT connection status (if enabled)

**Monthly**:
- Review configuration settings
- Check battery level (if using battery power)
- Verify WiFi connection stability
- Review syslog output for errors (if enabled)
- Test backup/restore functionality

### Configuration Backup

**Via Web Interface**:
1. Access Status page
2. Click "Download Backup" in Device Management section
3. Save JSON file to secure location
4. Backup includes: WiFi, FLEX settings, IMAP, MQTT, ChatGPT, Grafana, Syslog, System Alerts

**Via AT Commands** (partial — full backup/restore is web-interface only):
```bash
AT+FREQ?          # Check current transmit frequency
AT+POWER?         # Check current transmit power
AT+FREQPPM?       # Check PPM correction
AT+FLEX?          # Check default FLEX settings (capcode, frequency, power)
AT+WIFI?          # Check WiFi status
AT+DEVICE?        # Check overall device/network/API/Grafana status
```

**Backup Contents** (JSON format):
- Device settings (banner, version)
- WiFi configuration (SSID, static IP if configured)
- FLEX settings (frequency, power, capcode, PPM correction)
- API settings (username, enabled status)
- IMAP configuration (server, credentials, check interval)
- MQTT configuration (broker, credentials, topics, certificates)
- ChatGPT settings (API key, scheduled prompts)
- Grafana webhook configuration
- Remote syslog settings
- System alerts configuration

### Factory Reset

**When Needed**:
- Forgotten WiFi password
- Corrupted configuration
- Changing network environment
- Clearing IMAP/MQTT/ChatGPT settings

**How to Reset**:
1. **Via Web Interface**: Status page → Factory Reset button (clears NVS + SPIFFS)
2. **Via AT Commands**: Send `AT+FACTORYRESET`
3. **Via Hardware**: Hold BOOT button for 30 seconds

**What Gets Reset**:
- All NVS settings (WiFi, FLEX, API)
- All SPIFFS files (IMAP, MQTT certificates, ChatGPT prompts)
- Device returns to AP mode with MAC-based password
- All integration settings cleared

After factory reset, repeat the initial WiFi setup process.

### Firmware Updates

**Current Version**: see `+DEVICE_FIRMWARE` in `AT+DEVICE?`'s response, sourced from `version.h`

**Update Process**:
1. Download latest firmware from project repository
2. Backup current configuration via web interface
3. Flash new firmware using Arduino IDE (see [FIRMWARE.md](FIRMWARE.md))
4. Restore configuration from backup file
5. Verify all features working correctly

**Features Included**:
- IMAP email-to-pager gateway
- MQTT bidirectional messaging with persistent sessions
- ChatGPT scheduled prompts
- Grafana webhook integration
- Remote syslog logging (RFC 3164)
- Persistent SPIFFS log system with web UI, AT commands, and REST endpoint
- RTC time integration for immediate boot timestamps
- Enhanced PPM correction precision (0.02 decimals)
- Improved watchdog stability
- Memory optimization (chunked HTTP responses)
- Security hardening (XSS protection, MAC-based passwords)

## 🚨 Troubleshooting

**🔧 Complete Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive issue resolution covering all web interface, WiFi, and device problems.

### Quick Web Interface Issues

**Can't access web interface**:
1. Check device IP address on OLED display
2. Verify you're on the same WiFi network
3. Try different browser or clear cache
4. Ping the device IP to test connectivity

**Page loads slowly**:
- Check WiFi signal strength
- Try connecting closer to device
- Restart your browser
- Check free heap memory on Status page

**Messages not sending**:
1. Verify all form fields are filled correctly
2. Check capcode format (numeric only)
3. Ensure frequency is within valid range (400-1000 MHz)
4. Check antenna connection
5. If frequency appears offset in SDR: Use PPM correction in Configuration page (0.02 precision)

### Device Issues

**OLED display blank**:
- Normal after 5 minutes of inactivity
- Press any button or send command to wake
- Check power connection

**WiFi connection lost**:
1. Check OLED for "Disconnected" status
2. Restart device (power cycle)
3. Reconfigure WiFi if needed

**Device not responding**:
1. Check power connections
2. Try serial connection with AT commands
3. Check watchdog status in logs
4. Factory reset if necessary

### Integration Issues

**IMAP not receiving emails**:
1. Verify email credentials (use app-specific password for Gmail)
2. Check IMAP server and port (993 for SSL)
3. Review IMAP status on Status page
4. Check free heap memory (low memory can cause failures)
5. Verify email format (subject line becomes message)

**MQTT connection fails**:
1. Verify broker address and port
2. Check username/password credentials
3. Review MQTT status on Status page
4. For TLS: ensure certificate uploaded correctly
5. Check persistent session status

**ChatGPT prompts not executing**:
1. Verify OpenAI API key is correct
2. Check internet connectivity
3. Review ChatGPT status on Status page
4. Verify prompt is enabled and scheduled correctly
5. Check free heap memory

**Grafana webhooks not working**:
1. Verify webhook endpoint URL is correct
2. Check authentication token if configured
3. Review webhook format from Grafana
4. Test with manual curl command
5. Check serial logs for webhook errors

**Syslog not sending**:
1. Verify syslog server address and port
2. Check transport protocol (UDP/TCP)
3. Test network connectivity to syslog server
4. Review severity filter level
5. Check serial logs for syslog errors

### Network Issues

**Device IP changed**:
- Check router DHCP settings
- Configure static IP in device settings
- Update bookmarks with new IP

**Can't find device on network**:
- Check router's connected device list
- Use network scanner to find ESP32 devices
- Verify device is connected to correct network

**High memory usage**:
- Disable unused integrations (IMAP, MQTT, ChatGPT)
- Reduce IMAP check frequency
- Limit number of active ChatGPT prompts
- Review Status page heap percentage

## 📚 Related Documentation

### Getting Started
- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message

### Detailed Technical References
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference with syntax, parameters, and examples
- **[REST_API.md](REST_API.md)**: Comprehensive REST API documentation with programming examples
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware installation and upgrade procedures

### Project Information
- **[README.md](../README.md)**: Project overview and quick start guide
- **[CLAUDE.md](../CLAUDE.md)**: Technical architecture and development notes

### Hardware-Specific Information
- **Board Selection**: This is a compile-time build flag, not an edit to the `.ino` file —
  select the target board via `./scripts/flex-build-upload.sh -t ttgo` / `-t heltec` (arduino-cli
  path) or the matching `pio run -e ttgo-*` / `-e heltec-*` environment (PlatformIO path); both
  set `TTGO_LORA32_V21` or `HELTEC_WIFI_LORA32_V2` as a compiler define. See [FIRMWARE.md](FIRMWARE.md).
- **Pin Definitions**: See `include/boards/boards.h` for master hardware-specific pin mappings
- **Hardware Details**: See main [README.md](../README.md) for supported hardware specifications

## 🆘 Getting Help

**🔧 Need Detailed Help?** See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for:
- Complete web interface troubleshooting procedures
- WiFi connectivity and network issue resolution
- Hardware debugging and recovery procedures
- Integration troubleshooting (IMAP, MQTT, ChatGPT, Grafana, Syslog)
- Professional GitHub issue reporting process

### Self-Help Resources
1. **Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)**: Comprehensive problem resolution guide
2. **Check the OLED display**: Often shows current status or error messages
3. **Review Status page**: Shows detailed system state and integration status
4. **Check serial logs**: Connect via USB and monitor serial output (115200 baud)
5. **Try different interface**: If web fails, try AT commands via serial
6. **Factory reset**: When in doubt, reset to known good state

### Still Need Help?
Follow the GitHub issue reporting process detailed in [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for professional technical support.

---

**Happy Paging!** 📡 This user guide covers everything you need to operate your FLEX paging message transmitter effectively, including IMAP email-to-pager, MQTT bidirectional messaging, ChatGPT scheduled prompts, Grafana webhooks, and remote syslog logging. For advanced usage and integration, explore the detailed technical documentation referenced throughout this guide.
