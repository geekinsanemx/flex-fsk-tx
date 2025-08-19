# TTGO FLEX Transmitter - User Guide

Complete user guide for operating the TTGO FLEX paging transmitter through all available interfaces (v3 firmware).

> **Note**: This guide covers the web interface and advanced features available in v3 firmware. For basic operation and AT commands compatible with all firmware versions, see [AT_COMMANDS.md](AT_COMMANDS.md).

## 🚀 Getting Started

### Initial Setup
1. **Power On**: Connect USB power or insert battery
2. **Display Check**: OLED should show device information
3. **Network Connection**: Device starts in AP mode for first-time setup
4. **Web Access**: Connect to TTGO_FLEX_XXXX network (password: 12345678)
5. **Configuration**: Open browser to http://192.168.4.1

### Quick First Message
1. Navigate to main page (message interface)
2. Enter frequency (e.g., 929.6625 MHz)
3. Set TX power (e.g., 10 dBm)
4. Enter capcode (e.g., 1234567)
5. Type your message
6. Click "Send Message"

## 📱 User Interfaces

### 1. OLED Display

The 128x64 pixel display shows real-time device status:

#### Normal Operation Mode
```
flex-fsk-tx          # Custom banner (configurable)
WiFi: MyNetwork      # Network name or "AP Mode"  
IP: 192.168.1.100    # Device IP address
Pwr: 10 dBm // 85%   # TX power and battery %
```

#### Access Point Mode
```
flex-fsk-tx
AP: TTGO_FLEX_A1B2
IP: 192.168.4.1
Pass: 12345678
```

#### Transmission Mode
```
flex-fsk-tx
TRANSMITTING
929.6625 MHz
Pwr: 10 dBm
```

#### Battery Information
- **Battery Present**: Shows voltage and percentage
- **External Power**: Shows "Pwr: XX dBm" only
- **Low Battery**: Voltage displayed in red (if <3.3V)

### 2. Web Interface

#### Main Message Page (/)

**URL**: `http://device-ip/` or `http://device-ip/message`

**Layout**: Two-column responsive design with real-time validation

**Fields**:
- **Frequency (MHz)**: Range 400.0-1000.0, step 0.0001
- **TX Power (0-20 dBm)**: Integer values, real-time validation
- **Capcode**: 1 to 4,294,967,295
- **Message**: Up to 240 characters with live counter
- **Mail Drop**: Per-message checkbox option

**Features**:
- Real-time character counter
- Input validation with visual feedback
- Form submission prevention for invalid values
- Success/error status messages
- Responsive design for mobile devices

**Example Usage**:
1. Set frequency: `929.6625`
2. Set power: `10`
3. Set capcode: `1234567`
4. Check mail drop if needed
5. Enter message: `Hello from TTGO FLEX!`
6. Click "Send Message"

#### Configuration Page (/configuration)

**URL**: `http://device-ip/configuration`

**Sections**:

##### Device Settings
- **Banner Message**: 1-16 characters for OLED display
- **Color Theme**: Default (Blue), Light, Dark

##### WiFi Settings
- **WiFi Enable**: Enable/Disable WiFi functionality
- **Use DHCP**: Automatic vs Static IP configuration
- **SSID**: Network name (32 characters max)
- **Password**: Network password (64 characters max)

##### IP Settings (when Static IP selected)
- **IP Address**: Device IP (e.g., 192.168.1.100)
- **Netmask**: Subnet mask (e.g., 255.255.255.0)
- **Gateway**: Network gateway (e.g., 192.168.1.1)
- **DNS Server**: DNS server (e.g., 8.8.8.8)

##### Default FLEX Settings
- **Default Frequency**: Startup frequency in MHz
- **Default TX Power**: Startup power (0-20 dBm)
- **Default Capcode**: Default target capcode

##### API Settings
- **API Listening Port**: REST API port (1024-65535)
- **API Username**: Authentication username (32 chars max)
- **API Password**: Authentication password (64 chars max)

**Features**:
- Two-column responsive layout
- Real-time validation
- Dynamic field enabling/disabling
- Automatic restart after configuration save

#### Status Page (/status)

**URL**: `http://device-ip/status`

**Device Information** (Two-column layout):

*Left Column*:
- **Banner**: Current banner message
- **Frequency**: Current transmission frequency
- **TX Power**: Current power setting
- **Default Capcode**: Configured default capcode
- **Uptime**: Device runtime in minutes
- **Free Heap**: Available system memory

*Right Column*:
- **Power/Battery**: Power source and battery status
- **Theme**: Current UI theme
- **WiFi Status**: Enabled/Disabled state
- **API Port**: Current API listening port
- **Chip Model**: ESP32 chip information
- **CPU Frequency**: Processor speed

**Network Information**:

*WiFi Connected*:
- WiFi SSID, IP Address, Subnet Mask
- Gateway, DNS Server, MAC Address
- RSSI (signal strength), DHCP status

*AP Mode*:
- Mode description, AP SSID, AP IP
- AP MAC Address, Connected clients count

*WiFi Disabled*:
- Status and MAC address only

**Factory Reset Section**:
- Warning message
- Safety checkbox: "I know what I'm doing!"
- Factory Reset button (enabled only when checkbox checked)
- Confirmation dialog before reset

### 3. AT Command Interface

**Connection**: Serial monitor at 115200 baud

#### Basic Commands

**Version Information**:
```
AT+VERSION
Response: +VERSION: TTGO FLEX Transmitter v3.0
          OK
```

**Frequency Control**:
```
AT+FREQ=929.6625     # Set frequency in MHz
AT+FREQ?             # Query current frequency
Response: +FREQ: 929.6625
          OK
```

**Power Control**:
```
AT+POWER=10          # Set power in dBm (0-20)
AT+POWER?            # Query current power
Response: +POWER: 10
          OK
```

**Capcode Configuration**:
```
AT+MSG=1234567       # Send FLEX message to capcode
                     # (then type message and press Enter)
Response: +CAPCODE: 1234567
          OK
```

#### Message Transmission

**Simple Message**:
```
AT+MSG=1234567
# Wait for "+MSG: READY" response
Hello World
Response: SENDING
          OK
```

**Message with Mail Drop**:
```
AT+MAILDROP=1        # Enable mail drop
AT+MSG=1234567
Urgent Message
Response: SENDING
          OK
```

**Binary Message Mode**:
```
AT+SEND=240          # Send 240 bytes of binary data
# Send exactly 240 bytes of data
Response: SENDING
          OK
```

#### System Commands

**WiFi Control**:
```
AT+WIFI=MyNetwork,MyPassword  # Configure and connect to WiFi
AT+WIFI?                      # Check WiFi status
```

**Configuration**:
```
AT+BANNER=MyDevice        # Set banner message (max 16 chars)
AT+APIPORT=16180          # Set API port
AT+APIUSER=admin          # Set API username
AT+APIPASS=password       # Set API password
```

**System Information**:
```
AT+STATUS?                # Show device status
AT+BATTERY?               # Show battery information
AT+SAVE                   # Save configuration to EEPROM
AT+FACTORYRESET           # Reset to defaults
```

**Response Codes**:
- `OK`: Command successful
- `ERROR`: Command failed
- `SENDING`: Message transmission started
- `READY`: Device ready for next command

## 🔧 Configuration Guide

### WiFi Setup

#### Method 1: Web Interface
1. Connect to device AP (TTGO_FLEX_XXXX)
2. Go to Configuration page
3. Enter WiFi credentials
4. Choose DHCP or Static IP
5. Save configuration
6. Device restarts and connects

#### Method 2: AT Commands
```
AT+WIFI=YourNetwork,YourPassword
```

#### Method 3: Static IP Configuration
Web interface → IP Settings:
- Uncheck "Use DHCP"
- Enter IP Address: 192.168.1.100
- Enter Netmask: 255.255.255.0
- Enter Gateway: 192.168.1.1
- Enter DNS: 8.8.8.8

### Frequency Planning

**Legal Considerations**:
- Verify local amateur radio or ISM band regulations
- Common paging frequencies:
  - 929.6625 MHz (US)
  - 931.0000 MHz (US)
  - 929.6375 MHz (US)

**Setting Frequency**:
- Web: Enter in frequency field (MHz)
- AT: `AT+FREQ=929.6625` and `AT+MSG=1234567` then type message
- Range: 400.0-1000.0 MHz
- Resolution: 0.0001 MHz (100 Hz steps)

### Power Management

**Power Settings**:
- Range: 0-20 dBm
- Typical: 10 dBm for local coverage
- Maximum: 20 dBm (100mW) for long range

**Battery Optimization**:
- Lower power = longer battery life
- OLED timeout: 30 seconds default
- WiFi disable when not needed

### Message Format

**FLEX Protocol Limits**:
- Maximum length: 240 characters
- Character set: 7-bit ASCII
- Mail drop: Optional per-message flag

**Best Practices**:
- Keep messages concise
- Use standard ASCII characters
- Test with short messages first

## 🎨 Customization

### Banner Messages
- Length: 1-16 characters
- Displayed on OLED top line
- Examples: "My FLEX TX", "KD8ABC", "Node-01"

### UI Themes

**Default (Blue)**:
- Blue headers and accents
- Light backgrounds
- Professional appearance

**Light Theme**:
- Minimal colors
- High contrast
- Clean appearance

**Dark Theme**:
- Dark backgrounds
- Light text
- Reduced eye strain

### Display Timeout
- Default: 30 seconds
- OLED turns off automatically
- Any activity wakes display

## 🛠️ Troubleshooting

### Common Issues

**Problem**: Device not responding
**Solutions**:
1. Check power connection
2. Press reset button
3. Verify USB cable
4. Check serial monitor (115200 baud)

**Problem**: Cannot connect to WiFi
**Solutions**:
1. Verify SSID and password
2. Check 2.4GHz network (5GHz not supported)
3. Try closer to router
4. Factory reset if needed

**Problem**: Transmission not working
**Solutions**:
1. Check antenna connection
2. Verify frequency is legal
3. Ensure power setting is appropriate
4. Check capcode format

**Problem**: Web interface not loading
**Solutions**:
1. Verify IP address
2. Check device is on same network
3. Try http:// prefix
4. Clear browser cache

### Error Messages

**Web Interface Errors**:
- "TX Power must be between 0 and 20 dBm"
- "Frequency must be between 400.0 and 1000.0 MHz"
- "Message length must be between 1 and 240 characters"
- "Device is busy"

**AT Command Errors**:
- "ERROR" - Invalid command or parameter
- "BUSY" - Device currently transmitting
- "INVALID FREQUENCY" - Frequency out of range
- "INVALID POWER" - Power out of range

### Factory Reset

**Hardware Reset**:
1. Hold BOOT button for 30 seconds
2. Device shows "FACTORY RESET" on display
3. Release button
4. Device restarts with defaults

**Web Interface Reset**:
1. Go to Status page
2. Check "I know what I'm doing!"
3. Click Factory Reset button
4. Confirm in dialog

**AT Command Reset**:
```
AT+FACTORYRESET
```

**Default Settings After Reset**:
- WiFi: Disabled
- Frequency: 929.6625 MHz
- Power: 10 dBm
- Capcode: 1234567
- Banner: "flex-fsk-tx"
- Theme: Default (Blue)
- API Port: 16180

## 📊 Performance Monitoring

### Status Indicators

**OLED Display**:
- Solid display: Normal operation
- Blank display: Sleep mode or error
- "TRANSMITTING": Active transmission

**LED Heartbeat**:
- 2 blinks every minute: Normal operation
- Solid on: Transmitting
- Off: Device not powered

**Web Interface**:
- Green "SUCCESS": Message sent
- Red "ERROR": Transmission failed
- Yellow "Sending...": In progress

### System Health

**Memory Usage**:
- Check "Free Heap" in status page
- Normal: >100KB available
- Warning: <50KB available
- Critical: <10KB available

**Network Performance**:
- RSSI: Signal strength (-30 = excellent, -70 = poor)
- Connected clients: Number of devices on AP
- Uptime: Device stability indicator

## 🔒 Security Best Practices

### Network Security
- Change default API credentials immediately
- Use strong WiFi passwords (WPA2)
- Implement network isolation if needed
- Monitor access logs

### Access Control
- Restrict API access to authorized users only
- Use HTTPS proxy if exposing to internet
- Implement IP filtering if required
- Regular security updates

### Operational Security
- Verify local transmission regulations
- Use appropriate power levels
- Monitor for interference
- Keep firmware updated

---

**Next Steps**: See [API.md](API.md) for REST API integration guide