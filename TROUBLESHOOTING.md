# FLEX Paging Message Transmitter - Comprehensive Troubleshooting Guide

Complete troubleshooting reference for the FLEX paging message transmitter system, covering all firmware versions and common issues.

## 🎯 Quick Issue Resolution

**First-time user?** Start with [QUICKSTART.md](QUICKSTART.md) for guided setup.

### Most Common Issues
1. **Device not detected** → Check USB cable and drivers
2. **Firmware upload fails** → See [FIRMWARE.md](FIRMWARE.md) for detailed upload procedures
3. **AT commands not working** → Check baud rate (115200) and line endings
4. **Web interface not accessible** → Verify WiFi connection and device IP
5. **No transmission** → Check antenna connection and frequency settings

---

## 🔧 Hardware Issues

### Device Not Detected

**Symptoms**: Computer doesn't recognize ESP32 device

**Solutions**:
1. **USB Cable Issues**:
   ```bash
   # Test with different cable - must support data transfer
   # Charging-only cables won't work
   ```

2. **Driver Installation**:
   ```bash
   # Linux - Install CH340/CH341 drivers
   sudo apt install ch341-uart-dkms
   
   # Check device detection
   dmesg | tail -10
   ls /dev/tty*
   ```

3. **Port Identification**:
   - **TTGO LoRa32-OLED**: Usually `/dev/ttyACM0` (Linux) or `COM3+` (Windows)
   - **Heltec WiFi LoRa 32 V3**: Usually `/dev/ttyUSB0` (Linux) or `COM4+` (Windows)

4. **Hardware Reset**:
   - Press RESET button on device
   - Try different USB port
   - Check for loose connections

### Permission Denied (Linux/macOS)

**Symptoms**: "Permission denied" when accessing serial port

**Solutions**:
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Apply changes immediately
newgrp dialout

# Or log out and back in
```

### Power Issues

**Symptoms**: Device doesn't power on or resets unexpectedly

**Solutions**:
1. **Power Supply**:
   - Use high-quality USB cable
   - Try different USB port (USB 3.0 preferred)
   - Check battery voltage (3.7V Li-Po)

2. **Current Draw**:
   - Some USB ports can't provide enough current
   - Try powered USB hub
   - Check for short circuits

---

## 📡 Firmware and Upload Issues

### Firmware Upload Failed

**Symptoms**: Arduino IDE upload fails with connection errors

**Complete Solutions**: See [FIRMWARE.md](FIRMWARE.md) for detailed upload troubleshooting procedures including:
- Manual boot mode procedures
- Board configuration settings
- Upload speed optimization
- Device-specific troubleshooting

**Quick Fixes**:
1. **Manual Boot Mode**:
   - Hold **BOOT** button on device
   - Press **RESET** button briefly
   - Release **RESET** button
   - Release **BOOT** button
   - Click Upload in Arduino IDE immediately

2. **Upload Settings**:
   - Lower upload speed to 115200
   - Try different USB cable
   - Restart Arduino IDE

### Compilation Errors

**Symptoms**: Code won't compile in Arduino IDE

**Complete Solutions**: See [FIRMWARE.md](FIRMWARE.md) for comprehensive library installation and compilation troubleshooting.

**Common Library Issues**:
```bash
# Missing tinyflex.h (v2/v3 firmware)
# Copy file to firmware directory:
cp include/tinyflex/tinyflex.h "Devices/TTGO LoRa32-OLED/"

# Missing RadioBoards (TTGO devices)
cd ~/Arduino/libraries/
git clone https://github.com/radiolib-org/RadioBoards.git
```

### Radio Initialization Failures

**Symptoms**: Device starts but radio doesn't initialize

**Diagnostic Commands**:
```bash
# Check AT command response
AT
# Expected: OK

# Check device status
AT+STATUS?
# Expected: +STATUS: READY or +STATUS: ERROR
```

**Solutions**:
1. **SX1262 Issues (Heltec V3)**:
   - Check antenna connection
   - Verify 3.3V power supply stability
   - Try different frequency: `AT+FREQ=915.0`

2. **SX1276 Issues (TTGO)**:
   - Check RadioBoards library installation
   - Verify board selection in Arduino IDE
   - Test with default frequency: `AT+FREQ=915.0`

3. **General Radio Issues**:
   - Never operate without antenna connected
   - Check for damaged radio module
   - Try factory reset: `AT+FACTORYRESET`

---

## 🌐 WiFi and Network Issues (v3 Firmware)

### Can't Connect to Device AP Mode

**Symptoms**: Can't find or connect to "TTGO_FLEX_XXXX" network

**Solutions**:
1. **AP Mode Activation**:
   ```bash
   # Force AP mode via AT command
   AT+WIFIENABLE=0  # Disable WiFi
   AT+WIFIENABLE=1  # Re-enable (starts AP mode)
   
   # Check AP status
   AT+WIFI?
   # Should show: +WIFI: AP_MODE or similar
   ```

2. **Network Visibility**:
   - Wait 60 seconds for AP to start
   - Check 2.4GHz WiFi capability on connecting device
   - Look for network named `TTGO_FLEX_XXXX`
   - Default password: `12345678`

3. **Connection Issues**:
   - Try connecting from different device
   - Clear WiFi cache on connecting device
   - Power cycle ESP32 device

### WiFi Authentication Failures

**Symptoms**: Device can't connect to home/office WiFi

**Diagnostic Output** (Serial Monitor):
```
WiFi authentication failed, starting AP mode
WiFi connection timeout, starting AP mode
WiFi timeout retry attempt: X
```

**Solutions**:
1. **Credential Verification**:
   ```bash
   # Check WiFi settings
   AT+WIFICONFIG?
   
   # Reconfigure with correct credentials
   AT+WIFI=YourNetworkName,YourPassword
   
   # Force reconnection
   AT+WIFICONNECT
   ```

2. **Network Compatibility**:
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
   - Check WPA2 security (WPA3 may not work)
   - Verify SSID doesn't contain special characters
   - Check network allows new device connections

3. **Signal Strength**:
   - Move device closer to router
   - Check for interference sources
   - Try different WiFi channel

### Web Interface Not Accessible

**Symptoms**: Can't access web interface at device IP

**Diagnostic Steps**:
1. **IP Address Verification**:
   ```bash
   # Check device IP via AT command
   AT+WIFI?
   # Response: +WIFI: CONNECTED,192.168.1.100
   
   # Check from device display (OLED screen)
   # Should show IP address on screen
   ```

2. **Network Connectivity**:
   ```bash
   # Test ping connectivity
   ping 192.168.1.100
   
   # Test port accessibility (v3 firmware)
   telnet 192.168.1.100 80     # Web interface
   telnet 192.168.1.100 16180  # REST API
   ```

**Solutions**:
1. **Same Network Verification**:
   - Ensure computer and device on same WiFi network
   - Check for network isolation/guest network restrictions
   - Try accessing from different device

2. **Browser Issues**:
   - Clear browser cache and cookies
   - Try different browser
   - Disable browser extensions
   - Try incognito/private mode

3. **Firewall/Security**:
   - Temporarily disable firewall
   - Check antivirus blocking connections
   - Verify router settings allow device communication

---

## 💻 AT Command Issues

### No Response to AT Commands

**Symptoms**: Device doesn't respond to any AT commands

**Diagnostic Steps**:
1. **Connection Verification**:
   ```bash
   # Test basic connectivity
   echo "AT" | screen /dev/ttyUSB0 115200
   
   # Check for any response
   timeout 5 screen /dev/ttyUSB0 115200
   ```

**Solutions**:
1. **Serial Settings**:
   - **Baud Rate**: Must be 115200
   - **Line Endings**: Set to "Both NL & CR"
   - **Data Bits**: 8
   - **Parity**: None
   - **Stop Bits**: 1

2. **Device State**:
   ```bash
   # Reset device
   # Press RESET button or power cycle
   
   # Try simple AT command
   AT
   # Should respond: OK
   ```

3. **Software Issues**:
   - Close other applications using serial port
   - Try different terminal software
   - Restart computer if needed

### AT Commands Return ERROR

**Symptoms**: Commands recognized but return "ERROR" response

**Common Causes and Solutions**:

1. **Parameter Out of Range**:
   ```bash
   # Frequency range: 400.0 - 1000.0 MHz
   AT+FREQ=1200.0  # ERROR - out of range
   AT+FREQ=915.0   # OK - within range
   
   # Power range varies by device:
   # TTGO: 0-20 dBm, Heltec: -9 to +22 dBm
   AT+POWER=25     # ERROR - too high
   AT+POWER=10     # OK - safe value
   ```

2. **Invalid Command Format**:
   ```bash
   # Incorrect format
   AT+FREQ 915.0   # ERROR - missing =
   AT+FREQ=915.0   # OK - correct format
   
   # Invalid parameters
   AT+MSG=ABC      # ERROR - capcode must be numeric
   AT+MSG=1234567  # OK - valid capcode
   ```

3. **Device Busy**:
   ```bash
   # Check device status
   AT+STATUS?
   # If TRANSMITTING, wait for completion
   
   # Abort current operation if needed
   AT+ABORT
   ```

4. **Firmware Version Mismatch**:
   ```bash
   # v1 firmware doesn't support:
   AT+MSG=1234567  # ERROR - not available in v1
   
   # v2+ firmware required for:
   AT+MSG=1234567  # OK in v2/v3
   AT+MAILDROP=1   # OK in v2/v3
   
   # v3 firmware required for:
   AT+WIFI?        # OK in v3 only
   AT+APIPORT?     # OK in v3 only
   ```

### AT+MSG Not Recognized

**Symptoms**: "ERROR" response to AT+MSG command

**Solutions**:
1. **Firmware Version Check**:
   - AT+MSG requires v2 or v3 firmware
   - See [FIRMWARE.md](FIRMWARE.md) for upgrading firmware
   - v1 firmware only supports AT+SEND (binary data)

2. **Command Sequence**:
   ```bash
   # Correct v2/v3 usage
   AT+MSG=1234567
   # Wait for: +MSG: READY
   Hello World!    # Type message and press Enter
   # Response: OK (message transmitted)
   ```

### WiFi Commands Not Working

**Symptoms**: WiFi-related AT commands return "ERROR"

**Solutions**:
1. **Firmware Requirements**:
   - WiFi commands require v3 firmware only
   - v1/v2 firmware doesn't support WiFi functionality
   - See [FIRMWARE.md](FIRMWARE.md) for v3 firmware installation

2. **WiFi Status Check**:
   ```bash
   # Check if WiFi is enabled
   AT+WIFIENABLE?
   # Should return: +WIFIENABLE: 1
   
   # Enable WiFi if disabled
   AT+WIFIENABLE=1
   ```

---

## 🎯 Message Transmission Issues

### No Transmission / Messages Not Sent

**Symptoms**: Commands succeed but no actual transmission occurs

**Diagnostic Steps**:
1. **Antenna Check**:
   - Verify antenna is connected
   - Check antenna is appropriate for frequency
   - Never operate without antenna (can damage radio)

2. **Device Status Monitoring**:
   ```bash
   # Check current state
   AT+STATUS?
   # Monitor state during transmission
   
   # For v2/v3 firmware with AT+MSG:
   AT+MSG=1234567
   # Watch OLED display for "Transmitting" status
   ```

**Solutions**:
1. **Radio Configuration**:
   ```bash
   # Verify frequency is set
   AT+FREQ?
   # Should return current frequency
   
   # Set appropriate frequency
   AT+FREQ=929.6625  # Common US frequency
   AT+FREQ=915.0     # ISM band
   
   # Check power setting
   AT+POWER?
   AT+POWER=10       # Safe starting power
   ```

2. **Message Format**:
   ```bash
   # Correct binary transmission (v1/v2/v3)
   AT+SEND=50
   # Send exactly 50 bytes of data
   
   # Correct FLEX message (v2/v3)
   AT+MSG=1234567
   # Wait for +MSG: READY
   # Type message (max 240 characters)
   # Press Enter
   ```

3. **Hardware Issues**:
   - Check antenna connection is secure
   - Verify appropriate antenna for frequency
   - Test with different power levels
   - Check for radio module damage

### Transmission Timeout / Device Stuck

**Symptoms**: Device gets stuck in transmitting state

**Diagnostic Signs**:
- OLED shows "Transmitting..." indefinitely
- AT+STATUS? returns TRANSMITTING for extended time
- Device doesn't respond to new commands

**Solutions**:
1. **Immediate Recovery**:
   ```bash
   # Abort current transmission
   AT+ABORT
   
   # Check status
   AT+STATUS?
   # Should return to READY
   ```

2. **Hardware Reset**:
   - Press RESET button on device
   - Power cycle device
   - Check antenna connection

3. **Prevent Future Issues**:
   - Use appropriate power levels (start with 5-10 dBm)
   - Ensure stable power supply
   - Avoid very long messages

### FLEX Encoding Errors (v2/v3 Firmware)

**Symptoms**: AT+MSG command fails with encoding errors

**Error Indicators**:
- Device state shows ERROR after AT+MSG
- Serial output shows encoding failures
- Message not transmitted

**Solutions**:
1. **Message Content**:
   ```bash
   # Valid message format
   AT+MSG=1234567
   # Wait for +MSG: READY
   Hello World 123!  # ASCII printable characters only
   
   # Avoid problematic characters
   # No extended ASCII or Unicode
   # Max 240 characters
   ```

2. **Capcode Validation**:
   ```bash
   # Valid capcode range: 1 to 4,294,967,295
   AT+MSG=1234567     # OK - valid range
   AT+MSG=0           # ERROR - too low
   AT+MSG=5000000000  # ERROR - too high
   ```

3. **tinyflex Library Issues**:
   - Ensure tinyflex.h is properly embedded in firmware
   - See [FIRMWARE.md](FIRMWARE.md) for embedding instructions
   - Try factory reset: `AT+FACTORYRESET`

---

## 🌐 REST API Issues (v3 Firmware Only)

### API Not Accessible

**Symptoms**: Can't connect to REST API on port 16180

**Diagnostic Steps**:
```bash
# Check if API port is accessible
telnet DEVICE_IP 16180

# Test basic connectivity
curl -v http://DEVICE_IP:16180/

# Check API configuration
AT+APIPORT?
AT+APIUSER?
```

**Solutions**:
1. **Port Configuration**:
   ```bash
   # Verify API port setting
   AT+APIPORT?
   # Default: +APIPORT: 16180
   
   # Change if needed
   AT+APIPORT=8080
   AT+SAVE  # Save to EEPROM
   ```

2. **Network Issues**:
   - Ensure device connected to WiFi
   - Check firewall blocking port 16180
   - Try different port number

### Authentication Failures

**Symptoms**: HTTP 401 Unauthorized responses

**Solutions**:
```bash
# Check current credentials
AT+APIUSER?
AT+APIPASS?

# Update credentials
AT+APIUSER=newuser
AT+APIPASS=newpassword
AT+SAVE

# Test with curl
curl -u newuser:newpassword http://DEVICE_IP:16180/
```

### JSON Payload Errors

**Symptoms**: HTTP 400 Bad Request responses

**Common Issues**:
```bash
# Invalid JSON format
curl -X POST http://DEVICE_IP:16180/ \
  -H "Content-Type: application/json" \
  -d "invalid json"  # ERROR

# Missing required fields
curl -X POST http://DEVICE_IP:16180/ \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567}'  # ERROR - missing frequency, power, message

# Correct format
curl -X POST http://DEVICE_IP:16180/ \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":10,"message":"Test"}'
```

---

## 📱 Display and Interface Issues

### OLED Display Problems

**Symptoms**: Display blank, garbled, or showing incorrect information

**Solutions**:
1. **Display Initialization**:
   - Power cycle device
   - Check for loose connections
   - Verify correct firmware for hardware

2. **TTGO Specific Issues**:
   ```bash
   # Ensure required libraries installed:
   # - U8g2 (display control)
   # - RadioBoards (board definitions)
   ```

3. **Heltec Specific Issues**:
   ```bash
   # Ensure Heltec ESP32 library installed
   # Verify board selection: "Heltec WiFi LoRa 32(V3)"
   ```

4. **Display Timeout**:
   - Display turns off after 5 minutes (normal behavior)
   - Send any AT command to wake display
   - Press RESET button to refresh display

### Status Display Issues

**Symptoms**: Incorrect status information on display

**Common Issues**:
1. **WiFi Status**: Shows "Disconnected" when connected
2. **Frequency/Power**: Shows old values after changes
3. **State**: Stuck showing "Transmitting"

**Solutions**:
```bash
# Refresh status information
AT+STATUS?

# Reset display timeout
# Send any command to refresh

# Factory reset if persistent
AT+FACTORYRESET
```

---

## 🔄 State Management Issues

### Device Stuck in Error State

**Symptoms**: AT+STATUS? returns +STATUS: ERROR

**Recovery Steps**:
```bash
# 1. Immediate recovery attempt
AT+ABORT
AT+STATUS?

# 2. Soft reset
AT+RESET

# 3. Factory reset (last resort)
AT+FACTORYRESET
```

### State Transitions Problems

**Symptoms**: Device doesn't transition between states properly

**Normal State Flow**:
1. `READY` → `WAITING_DATA` (after AT+SEND)
2. `READY` → `WAITING_MSG` (after AT+MSG)
3. `WAITING_*` → `TRANSMITTING` (during transmission)
4. `TRANSMITTING` → `READY` (after completion)

**Troubleshooting**:
```bash
# Monitor state transitions
AT+STATUS?
# Repeat during operation

# If stuck in WAITING state:
AT+ABORT  # Cancel operation

# If stuck in TRANSMITTING:
# Press RESET button (hardware reset)
```

---

## 🔋 Power and Performance Issues

### Device Resets Unexpectedly

**Symptoms**: Device restarts during operation

**Common Causes**:
1. **Insufficient Power Supply**:
   - Use high-quality USB cable
   - Try different USB port (USB 3.0 preferred)
   - Check battery voltage (3.7V for Li-Po)

2. **Brown-out Detection**:
   - High transmission power with weak power supply
   - Lower TX power: `AT+POWER=5`
   - Use powered USB hub

3. **Watchdog Timer Reset**:
   - Firmware crash or infinite loop
   - Update to latest firmware
   - Report issue with debug information

### Poor Transmission Range

**Symptoms**: Messages not received at expected distance

**Solutions**:
1. **Antenna Optimization**:
   - Use proper antenna for frequency
   - Check antenna orientation
   - Ensure good antenna connection

2. **Power Settings**:
   ```bash
   # Increase transmission power gradually
   AT+POWER=10   # Start conservative
   AT+POWER=15   # Increase if needed
   AT+POWER=20   # Maximum for TTGO
   
   # Check legal limits for your region
   ```

3. **Frequency Selection**:
   - Use optimal frequency for region
   - Check for interference
   - Try different frequencies within legal band

---

## 🔍 Diagnostic Information Collection

### Device Information Gathering

When reporting issues, collect this diagnostic information:

```bash
# Basic device information
AT
AT+STATUS?

# Configuration information
AT+FREQ?
AT+POWER?

# Version-specific information
# For v2/v3 firmware:
AT+MAILDROP?

# For v3 firmware only:
AT+WIFI?
AT+WIFICONFIG?
AT+APIPORT?
AT+APIUSER?
AT+BATTERY?

# Hardware test
AT+FREQ=915.0
AT+POWER=5
AT+MSG=1234567
Test message
```

### Arduino IDE Debug Information

For firmware compilation issues:

```bash
# Enable verbose output in Arduino IDE:
# File → Preferences → Show verbose output during: compilation ✓

# Include in issue report:
# 1. Complete error message
# 2. Board selection
# 3. Library versions
# 4. Arduino IDE version
# 5. Operating system

# Example diagnostic command:
arduino-cli board list
arduino-cli lib list
```

### Serial Monitor Output

Capture serial output during problem reproduction:

```bash
# Example session log:
$ screen /dev/ttyUSB0 115200
AT
OK
AT+STATUS?
+STATUS: READY
OK
AT+MSG=1234567
+MSG: READY
Hello World
OK
# End of session
```

---

## 🐛 GitHub Issue Submission

When you encounter issues that aren't resolved by this troubleshooting guide, please submit a detailed issue report to help improve the project.

### How to Submit Issues

1. **Navigate to GitHub Repository**:
   ```
   https://github.com/geekinsanemx/flex-fsk-tx/issues
   ```

2. **Click "New Issue"**

3. **Choose Issue Type**:
   - **Bug Report**: For software bugs, hardware issues, or unexpected behavior
   - **Feature Request**: For new functionality suggestions
   - **Documentation**: For unclear or missing documentation
   - **Question**: For usage questions not covered in documentation

### Issue Report Template

#### **Bug Reports**

```markdown
## Bug Description
Brief description of the issue

## Environment
- **Device**: TTGO LoRa32-OLED / Heltec WiFi LoRa 32 V3
- **Firmware Version**: v1 / v2 / v3
- **Host OS**: Windows 10 / macOS 14 / Ubuntu 22.04
- **Arduino IDE Version**: 2.x.x
- **Library Versions**:
  - RadioLib: x.x.x
  - U8g2: x.x.x (TTGO only)
  - ArduinoJson: x.x.x (v3 only)

## Steps to Reproduce
1. Step one
2. Step two
3. Step three

## Expected Behavior
What you expected to happen

## Actual Behavior
What actually happened

## Diagnostic Information
```bash
# Include output from:
AT+STATUS?
AT+FREQ?
AT+POWER?
# Add any error messages
```

## Serial Monitor Output
```
# Paste complete serial session showing the issue
```

## Additional Context
- Screenshots of error messages
- OLED display photos if relevant
- Network configuration details (for v3 firmware)
```

#### **Feature Requests**

```markdown
## Feature Description
Brief description of the requested feature

## Use Case
Why this feature would be useful

## Proposed Implementation
How you think it could be implemented (optional)

## Alternatives Considered
Other solutions you've considered
```

#### **Documentation Issues**

```markdown
## Documentation Problem
Which documentation is unclear or missing

## Suggested Improvement
How the documentation could be improved

## Affected Files
- [ ] README.md
- [ ] QUICKSTART.md
- [ ] USER_GUIDE.md
- [ ] AT_COMMANDS.md
- [ ] REST_API.md
- [ ] FIRMWARE.md
- [ ] Other: ____________
```

### Issue Guidelines

#### **Before Submitting**:
1. **Search existing issues** to avoid duplicates
2. **Try latest firmware version** - many issues are already fixed
3. **Follow troubleshooting guide** - ensure issue isn't covered here
4. **Test with minimal setup** - isolate the problem

#### **Writing Good Issues**:
1. **Be specific**: "AT+MSG doesn't work" vs "AT+MSG returns ERROR with capcode 1234567"
2. **Include context**: Operating system, firmware version, hardware variant
3. **Provide examples**: Show exact commands and responses
4. **Add diagnostics**: Include AT command output and serial logs

#### **What NOT to Include**:
- **Personal information**: Remove any sensitive data from logs
- **Unrelated issues**: One issue per report
- **Vague descriptions**: "It doesn't work" without details

### Response Expectations

- **Bug reports**: Typically receive response within 48-72 hours
- **Feature requests**: May take longer for evaluation
- **Documentation**: Usually quick fixes if straightforward

### Contributing to Solutions

If you solve an issue yourself:
1. **Share the solution** in issue comments
2. **Submit documentation improvements** via pull request
3. **Help others** with similar issues

---

## 📚 Documentation Resources

### Getting Started
- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message

### Technical References
- **[README.md](README.md)**: General project information and overview
- **[FIRMWARE.md](FIRMWARE.md)**: Comprehensive firmware installation and troubleshooting
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference with examples
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface user guide (v3 firmware)
- **[REST_API.md](REST_API.md)**: REST API documentation with programming examples

### Hardware-Specific
- **[Devices/TTGO LoRa32-OLED/README.md](Devices/TTGO%20LoRa32-OLED/README.md)**: TTGO-specific information
- **[Devices/Heltec LoRa32 V3/README.md](Devices/Heltec%20LoRa32%20V3/README.md)**: Heltec-specific information

---

## 🆘 Emergency Recovery Procedures

### Complete Device Reset

If device is completely unresponsive:

1. **Hardware Reset**:
   - Disconnect power (USB + battery)
   - Wait 10 seconds
   - Reconnect power
   - Press RESET button

2. **Factory Reset via AT Commands**:
   ```bash
   AT+FACTORYRESET
   # Device restarts with default settings
   ```

3. **Factory Reset via Hardware** (v3 firmware):
   - Hold BOOT button for 30 seconds
   - Device returns to AP mode with default settings

4. **Firmware Reflash**:
   - If all else fails, reflash firmware
   - See [FIRMWARE.md](FIRMWARE.md) for procedures
   - Choose appropriate firmware version for your needs

### Recovery Checklist

When all else fails, work through this checklist:

- [ ] **Power cycle device** (disconnect/reconnect)
- [ ] **Try different USB cable** (ensure data capability)
- [ ] **Test different USB port** (preferably USB 3.0)
- [ ] **Check device drivers** (especially on Windows)
- [ ] **Try different computer** (isolate host PC issues)
- [ ] **Reflash firmware** (see FIRMWARE.md)
- [ ] **Check hardware** (antenna, connections, damage)
- [ ] **Submit GitHub issue** (if problem persists)

---

**Still having issues?** Open a detailed issue on GitHub with the diagnostic information above: [https://github.com/geekinsanemx/flex-fsk-tx/issues](https://github.com/geekinsanemx/flex-fsk-tx/issues)