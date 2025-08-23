# FLEX Paging Message Transmitter - User Guide

Complete user guide for operating the FLEX paging message transmitter. This guide covers the user-friendly web interface, basic AT command usage, and REST API overview for the v3 firmware.

> **Hardware Requirement**: This guide is for **ESP32 LoRa32 devices with v3 firmware** (TTGO LoRa32-OLED or Heltec WiFi LoRa 32 V3). The web interface and WiFi features are only available in v3 firmware.

## 🚀 Getting Started

### What You'll Need

**Hardware**:
- ESP32 LoRa32 development board:
  - TTGO LoRa32-OLED (ESP32 + SX1276 radio)
  - Heltec WiFi LoRa 32 V3 (ESP32 + SX1262 radio)
- USB cable for initial setup
- Appropriate antenna for your frequency band

**Firmware**:
- v3 firmware must be installed on your ESP32 device
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

When you power on your ESP32 device with v3 firmware for the first time, it will create its own WiFi hotspot for configuration.

### Step 1: Connect to Device Hotspot

1. **Power on** your ESP32 device (via USB or battery)
2. **Wait 30 seconds** for the device to boot completely
3. **Check the OLED display** - it should show "flex-fsk-tx" banner
4. **Look for WiFi network** with device-specific name:
   - TTGO devices: `TTGO_FLEX_XXXX` (where XXXX is 4 hex characters)
   - Heltec devices: `HELTEC_FLEX_XXXX` (where XXXX is 4 hex characters)

**WiFi Network Details**:
- **Network Name**: 
  - `TTGO_FLEX_XXXX` (e.g., `TTGO_FLEX_A1B2`)
  - `HELTEC_FLEX_XXXX` (e.g., `HELTEC_FLEX_C3D4`)
- **Password**: `12345678`
- **Security**: WPA2

### Step 2: Access Configuration Portal

1. **Connect** your computer/phone to the device's WiFi network (`TTGO_FLEX_XXXX` or `HELTEC_FLEX_XXXX`)
2. **Open web browser** and navigate to:
   ```
   http://192.168.4.1
   ```
3. **Wait for page to load** - you'll see the FLEX Paging Message Transmitter configuration page

### Step 3: Configure Your WiFi Network

1. **Navigate to Configuration**: Click on "Configuration" or go to `http://192.168.4.1/configuration`

2. **WiFi Network Settings**:
   - **SSID**: Enter your home/office WiFi network name
   - **Password**: Enter your WiFi password
   - **DHCP**: Leave enabled (recommended) or configure static IP if needed

3. **Device Settings** (Optional):
   - **Device Banner**: Custom name displayed on OLED (max 16 characters)
   - **API Port**: REST API port (default: 16180)
   - **API Username**: Username for REST API access
   - **API Password**: Password for REST API access

4. **Save Configuration**: Click "Save Settings" button

### Step 4: Connection and Verification

1. **Device Restart**: The ESP32 device will automatically restart and connect to your WiFi
2. **Check OLED Display**: Should show "Connected" and IP address
3. **Note the IP Address**: Write down the IP shown on the display (e.g., `192.168.1.100`)
4. **Test Connection**: Open browser and go to `http://[DEVICE_IP]` (e.g., `http://192.168.1.100`)

### Troubleshooting Initial Setup

**Device hotspot not visible**:
- Wait longer (up to 2 minutes) for device to boot
- Check that v3 firmware is installed
- Power cycle the device

**Can't connect to 192.168.4.1**:
- Ensure you're connected to the device's WiFi network (TTGO_FLEX_XXXX or HELTEC_FLEX_XXXX)
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
   - Range: 1 to 4,294,967,295
   - Examples: `1234567`, `8901234567`

2. **Message** (Required):
   - Text message to send (up to 240 characters)
   - Character counter shows remaining space
   - Only printable ASCII characters supported

3. **Frequency** (Required):
   - Transmission frequency in MHz
   - Range: 400.0 to 1000.0 MHz
   - Common examples:
     - `929.6625` MHz (US common)
     - `931.9375` MHz (Alternative)
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

Access via `/configuration` or click "Configuration" link.

**WiFi Settings**:
- **Current Network**: Shows connected SSID and IP
- **Change Network**: Enter new SSID/password
- **Network Status**: Connection status and signal strength

**Device Settings**:
- **Banner Message**: Custom text for OLED display
- **API Port**: Port for REST API (1024-65535)
- **Authentication**: Username/password for API access

**System Settings**:
- **Factory Reset**: Reset all settings to defaults
- **Restart Device**: Reboot the device
- **Save Configuration**: Save current settings to EEPROM

### Status Page

Access via `/status` or click "Status" link.

**System Information**:
- **Uptime**: How long device has been running
- **Free Memory**: Available RAM
- **Chip Information**: ESP32 details and MAC address

**Network Status**:
- **WiFi Status**: Connection state and signal strength
- **IP Address**: Current network IP
- **API Status**: REST API availability

**Hardware Status**:
- **Battery Level**: Voltage and percentage (if battery connected)
- **Radio Status**: SX1276 radio module state
- **Temperature**: Device operating temperature

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
- Only one transmission can occur at a time
- Device shows "busy" status during transmission

**Offline Usage**:
- Web interface works without internet connection
- Only requires local network connectivity to device

## 🔌 Serial Interface Overview (AT Commands)

The ESP32 device also supports direct serial communication via AT commands for advanced users and automation.

### Basic AT Command Usage

**Connection Setup**:
- **Port**: USB serial port:
  - TTGO: typically `/dev/ttyACM0` (Linux), `COM3` (Windows)
  - Heltec: typically `/dev/ttyUSB0` (Linux), `COM4` (Windows)
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
http://[DEVICE_IP]:16180/
```

**Authentication**:
- HTTP Basic Authentication
- Default credentials: `username:password`
- Configurable via web interface or AT commands

**Simple Message Example**:
```bash
curl -X POST http://192.168.1.100:16180/ \
  -u username:password \
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

### API Features

**JSON Format**: Easy to use with any programming language
**Parameter Validation**: Server-side validation with detailed error messages
**Multiple Formats**: Supports both MHz and Hz frequency formats
**Error Handling**: Comprehensive HTTP status codes and error responses
**Rate Limiting**: Prevents device overload

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
| **Batch Operations** | No | Yes | Yes |

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

## 🔧 Maintenance and Updates

### Regular Maintenance

**Weekly**:
- Check OLED display for any error messages
- Verify web interface accessibility
- Test message transmission

**Monthly**:
- Review configuration settings
- Check battery level (if using battery power)
- Verify WiFi connection stability

### Configuration Backup

**Via Web Interface**:
1. Access Configuration page
2. Note down all settings
3. Save screenshots of configuration

**Via AT Commands**:
```bash
AT+WIFI?          # Check WiFi settings
AT+APIPORT?       # Check API port
AT+APIUSER?       # Check API username
AT+BANNER?        # Check banner setting
```

### Factory Reset

**When Needed**:
- Forgotten WiFi password
- Corrupted configuration
- Changing network environment

**How to Reset**:
1. **Via Web Interface**: Configuration page → Factory Reset button
2. **Via AT Commands**: Send `AT+FACTORYRESET`
3. **Via Hardware**: Hold BOOT button for 30 seconds

After factory reset, repeat the initial WiFi setup process.

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

**Messages not sending**:
1. Verify all form fields are filled correctly
2. Check capcode format (numeric only)
3. Ensure frequency is within valid range (400-1000 MHz)
4. Check antenna connection

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
3. Factory reset if necessary

### Network Issues

**Device IP changed**:
- Check router DHCP settings
- Configure static IP in device settings
- Update bookmarks with new IP

**Can't find device on network**:
- Check router's connected device list
- Use network scanner to find ESP32 devices
- Verify device is connected to correct network

## 📚 Related Documentation

### Getting Started
- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message

### Detailed Technical References
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference with syntax, parameters, and examples
- **[REST_API.md](REST_API.md)**: Comprehensive REST API documentation with programming examples
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware installation and upgrade procedures

### Project Information
- **[README.md](README.md)**: Project overview and quick start guide
- **[CLAUDE.md](CLAUDE.md)**: Technical architecture and development notes

### Hardware-Specific Guides
- **[Devices/TTGO LoRa32-OLED/README.md](Devices/TTGO%20LoRa32-OLED/README.md)**: TTGO-specific hardware information

## 🆘 Getting Help

**🔧 Need Detailed Help?** See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for:
- Complete web interface troubleshooting procedures
- WiFi connectivity and network issue resolution
- Hardware debugging and recovery procedures
- Professional GitHub issue reporting process

### Self-Help Resources
1. **Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)**: Comprehensive problem resolution guide
2. **Check the OLED display**: Often shows current status or error messages
3. **Try different interface**: If web fails, try AT commands via serial
4. **Factory reset**: When in doubt, reset to known good state

### Still Need Help?
Follow the GitHub issue reporting process detailed in [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for professional technical support.

---

**Happy Paging!** 📡 This user guide covers everything you need to operate your FLEX paging message transmitter effectively. For advanced usage and integration, explore the detailed technical documentation referenced throughout this guide.