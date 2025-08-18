# Troubleshooting Guide

Common issues and solutions for the flex-fsk-tx FLEX paging transmitter system.

## 🔧 Hardware Issues

### Device Not Detected

**Symptoms**: Computer doesn't recognize ESP32 device
**Solutions**:
- Check USB cable (try different cable)
- Verify correct serial port:
  - **Heltec V3**: `/dev/ttyUSB0` (Linux)
  - **TTGO**: `/dev/ttyACM0` (Linux)
- Install USB drivers if needed:
  ```bash
  sudo apt install ch341-uart-dkms  # For CH340/CH341 chips
  ```

### Permission Denied

**Symptoms**: "Permission denied" when accessing serial port
**Solutions**:
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in, or:
newgrp dialout
```

### Device Not Responding

**Symptoms**: No response to AT commands
**Solutions**:
- Press RESET button on ESP32
- Check serial connection settings (115200 baud, 8N1)
- Try different serial device (`/dev/ttyUSB1`, `/dev/ttyACM1`)
- Verify firmware is properly flashed

## 📡 Firmware Issues

### Upload Failed

**Symptoms**: Firmware upload fails in Arduino IDE
**Solutions**:
1. **Manual Boot Mode**:
   - Hold BOOT button
   - Press RESET button
   - Release RESET button
   - Release BOOT button
   - Try upload again

2. **Settings Check**:
   - Lower upload speed (115200)
   - Verify correct board selection:
     - Heltec: "Heltec WiFi LoRa 32(V3)"
     - TTGO: "TTGO LoRa32-OLED V1" or "ESP32 Dev Module"

3. **Cable/Port Issues**:
   - Try different USB cable
   - Try different USB port
   - Check for loose connections

### Compilation Errors

**Symptoms**: Code won't compile in Arduino IDE
**Solutions**:

**Missing Libraries**:
- Install via Library Manager:
  - RadioLib (all versions)
  - U8g2 (TTGO devices)
  - ArduinoJson (v3 firmware)
  - Heltec ESP32 library (Heltec devices)
- Manual install RadioBoards (v3 firmware):
  ```bash
  cd ~/Arduino/libraries/
  git clone https://github.com/radiolib-org/RadioBoards.git
  ```

**Board Not Found**:
- Install ESP32 board support:
  - File → Preferences → Additional Board URLs
  - Add: `https://dl.espressif.com/dl/package_esp32_index.json`
  - Tools → Board → Boards Manager → Search "ESP32" → Install

### Wrong Firmware Features

**Symptoms**: AT commands not recognized
**Solutions**:
- **AT+MSG not recognized**: Flash v2+ firmware
- **WiFi commands not recognized**: Flash v3 firmware (TTGO only)
- **Web interface not available**: Ensure v3 firmware on TTGO device

## 🌐 WiFi Issues (v3 Firmware)

### Can't Connect to Device AP

**Symptoms**: Can't find or connect to "TTGO_FLEX_XXXX" network
**Solutions**:
- Wait 30 seconds after power-on for AP to start
- Check device is in AP mode (OLED shows AP info)
- Try factory reset: `AT+FACTORYRESET` via serial
- Verify v3 firmware is flashed

### Web Interface Not Loading

**Symptoms**: Browser can't load `http://192.168.4.1` or device IP
**Solutions**:
- Verify WiFi connection to device
- Check device IP on OLED display
- Try different browser
- Clear browser cache
- Check firewall settings

### WiFi Won't Connect to Network

**Symptoms**: Device can't connect to home WiFi
**Solutions**:
```bash
# Via serial AT commands:
AT+WIFISSID=YourNetworkName
AT+WIFIPASS=YourPassword
AT+WIFICONNECT
```
- Verify SSID and password are correct
- Check network security (WPA2 supported)
- Ensure network allows new devices
- Try forgetting and re-adding network

### Lost Device IP Address

**Symptoms**: Don't know device IP after WiFi connection
**Solutions**:
- Check OLED display (shows current IP)
- Check router admin page for connected devices
- Use network scanner: `nmap -sn 192.168.1.0/24`
- Connect via serial and use `AT+STATUS?`

## 📻 Radio Issues

### No Transmission

**Symptoms**: Commands accepted but no RF output
**Solutions**:
- **Critical**: Check antenna connection (can damage radio if missing)
- Verify frequency is within legal limits
- Test with lower power settings first
- Check for RadioLib compilation errors
- Ensure correct radio chipset firmware (SX1262 vs SX1276)

### Transmission Errors

**Symptoms**: Error messages during transmission
**Solutions**:
- Check frequency range (400-1000 MHz)
- Verify power range (0-20 dBm for TTGO, -9 to +22 dBm for Heltec)
- Ensure message length ≤ 240 characters
- Wait for previous transmission to complete

## 🔄 Communication Issues

### AT Commands Fail

**Symptoms**: Commands return ERROR or no response
**Solutions**:
- Check command syntax (case-insensitive)
- Ensure proper line endings (CRLF)
- Wait for previous command to complete
- Verify parameter ranges
- Try basic `AT` command first

### Host Application Errors

**Symptoms**: flex-fsk-tx application fails
**Solutions**:
- Check serial device path
- Verify ESP32 is responding to AT commands
- Try different baud rate: `-b 115200`
- Check for other programs using serial port
- Build with debug: `make debug`

## 🔧 Host System Issues

### Build Failures

**Symptoms**: `make` command fails
**Solutions**:
- Check tinyflex submodule: `git submodule update --init --recursive`
- Install dependencies: `make check-deps`
- Install build tools: `sudo apt install build-essential`
- Check compiler: `gcc --version`

### Serial Port Detection

**Symptoms**: Can't find correct serial port
**Solutions**:
```bash
# List available ports
ls /dev/tty*

# Check when plugging device
dmesg | tail

# Common ports by device:
# Heltec V3: /dev/ttyUSB0, /dev/ttyUSB1
# TTGO: /dev/ttyACM0, /dev/ttyACM1
```

## 🔍 Diagnostic Commands

### Device Status Check
```bash
# Connect via serial
screen /dev/ttyACM0 115200

# Basic connectivity
AT
# Should return: OK

# Device information
AT+STATUS?
AT+VERSION
AT+BATTERY  # v3 firmware
```

### Network Diagnostics (v3)
```bash
# WiFi status
AT+WIFIENABLE?
AT+WIFICONNECT

# Network information
ping DEVICE_IP
curl -I http://DEVICE_IP/
```

### Radio Testing
```bash
# Set safe test parameters
AT+FREQ=916.0
AT+POWER=5

# Test transmission (v2+ firmware)
AT+MSG=1234567
# Type: Test message
# Should return: OK
```

## 🆘 Recovery Procedures

### Factory Reset (v3 Firmware)
```bash
# Via AT commands
AT+FACTORYRESET

# Via hardware (if available)
# Hold BOOT button for 30 seconds
```

### Firmware Recovery
1. Flash known-good firmware
2. Test with basic AT commands
3. Gradually add features

### Complete System Reset
1. Factory reset device
2. Rebuild host application: `make clean && make`
3. Re-test with basic commands
4. Reconfigure settings step by step

## 📞 Getting Help

### Information to Collect
- Device type (Heltec V3 or TTGO)
- Firmware version being used
- Host OS and version
- Serial port being used
- Exact error messages
- Steps that lead to the issue

### Useful Diagnostic Output
```bash
# System information
uname -a
lsusb
ls -la /dev/tty*

# Build information
make --version
gcc --version

# Arduino IDE: Include compilation output
```

### Documentation Resources
- **[README.md](README.md)**: General project information
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware setup details
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete command reference
- **[QUICKSTART.md](QUICKSTART.md)**: Fast setup guide

---

**Still having issues?** Open an issue on GitHub with the diagnostic information above.