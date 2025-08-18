# Quick Start Guide

**Get your FLEX transmitter running in under 30 minutes**

This guide provides the fastest path to sending your first FLEX message. For detailed information, see the comprehensive documentation linked throughout.

## 🎯 Choose Your Path

### Option A: Basic AT Commands (All Hardware)
- ✅ Works with any ESP32 LoRa32 device
- ✅ Supports all firmware versions (v1, v2, v3)
- ✅ Requires host computer running flex-fsk-tx application

### Option B: WiFi Web Interface (TTGO Only)
- ✅ Standalone operation without host computer
- ✅ Web browser control
- ✅ TTGO LoRa32-OLED with v3 firmware only

---

## 🚀 Option A: Basic AT Commands

### Step 1: Hardware Requirements
- **Heltec WiFi LoRa 32 V3** OR **TTGO LoRa32-OLED** development board
- USB cable and appropriate antenna
- Linux/Unix computer with USB port

### Step 2: Get the Code
```bash
git clone --recursive https://github.com/geekinsanemx/flex-fsk-tx.git
cd flex-fsk-tx
```

### Step 3: Flash Basic Firmware
1. Install Arduino IDE with ESP32 support
2. Install RadioLib library via Library Manager
3. For **Heltec**: Flash `Devices/Heltec LoRa32 V3/heltec_fsk_tx_AT.ino`
4. For **TTGO**: Flash `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT.ino`

> **Detailed instructions**: [FIRMWARE.md](FIRMWARE.md)

### Step 4: Build Host Application
```bash
make
sudo make install
```

### Step 5: Send First Message
```bash
# For Heltec (usually /dev/ttyUSB0)
flex-fsk-tx -d /dev/ttyUSB0 1234567 "Hello World"

# For TTGO (usually /dev/ttyACM0)
flex-fsk-tx -d /dev/ttyACM0 1234567 "Hello World"
```

**✅ Success!** Your device should transmit the message.

---

## 🌐 Option B: WiFi Web Interface (TTGO Only)

### Step 1: Hardware Requirements
- **TTGO LoRa32-OLED** development board (ESP32 + SX1276)
- USB cable and appropriate antenna
- Computer with Arduino IDE for initial firmware flash

### Step 2: Get v3 Firmware
```bash
git clone --recursive https://github.com/geekinsanemx/flex-fsk-tx.git
cd flex-fsk-tx
git checkout v3-standalone-web
```

### Step 3: Install Required Libraries
Via Arduino IDE Library Manager:
- **RadioLib** by Jan Gromeš
- **U8g2** by oliver
- **ArduinoJson** by Benoit Blanchon

Manual installation:
```bash
cd ~/Arduino/libraries/
git clone https://github.com/radiolib-org/RadioBoards.git
```

### Step 4: Flash v3 Firmware
1. Open `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT_v3.ino`
2. Select board: "TTGO LoRa32-OLED V1" or "ESP32 Dev Module"
3. Upload firmware

> **Detailed instructions**: [FIRMWARE.md](FIRMWARE.md)

### Step 5: Initial Setup
1. **Power on device** - OLED shows "flex-fsk-tx"
2. **Connect to WiFi AP**: "TTGO_FLEX_XXXX" (password: `12345678`)
3. **Open browser**: `http://192.168.4.1`
4. **Configure WiFi**: Enter your network details
5. **Connect**: Device restarts and joins your network

### Step 6: Send First Message
1. **Find device IP**: Check your router or OLED display
2. **Open web interface**: `http://DEVICE_IP/`
3. **Enter details**:
   - Frequency: 929.6625
   - Power: 10
   - Capcode: 1234567
   - Message: "Hello World"
4. **Click "Send Message"**

**✅ Success!** Your device transmits via web interface.

---

## 🔧 Quick Troubleshooting

### Device Not Responding
- Check USB cable and serial port
- Try different baud rate or serial device
- Press RESET button on ESP32

### Firmware Upload Failed
- Hold BOOT button, press RESET, release RESET, release BOOT
- Try lower upload speed (115200)
- Check board selection in Arduino IDE

### WiFi Connection Issues (v3)
- Connect to AP mode: "TTGO_FLEX_XXXX"
- Factory reset: `AT+FACTORYRESET` via serial

### Permission Denied (Linux)
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

---

## 📚 Next Steps

### Enhance Your Setup
- **v2 Firmware**: On-device FLEX encoding with `AT+MSG` commands
- **Advanced Features**: Mail drop, custom frequencies, power control
- **Automation**: REST API integration, scripting

### Documentation Reference
- **[README.md](README.md)**: Complete project overview
- **[FIRMWARE.md](FIRMWARE.md)**: Detailed firmware setup guide  
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference
- **[API.md](API.md)**: REST API documentation (v3 firmware)
- **[USER.md](USER.md)**: Web interface user guide (v3 firmware)

### Hardware-Specific Guides
- **[Devices/TTGO LoRa32-OLED/README.md](Devices/TTGO%20LoRa32-OLED/README.md)**: TTGO-specific information
- **[README_v3.md](README_v3.md)**: v3 firmware feature guide

---

## ⚠️ Safety Reminders

1. **Antenna Required**: Never operate without proper antenna
2. **Legal Compliance**: Verify frequency/power limits in your region
3. **Interference**: Monitor for interference to other services
4. **Power Limits**: Use minimum necessary power

---

**Need help?** Check the detailed documentation or open an issue on GitHub.