# Quick Start Guide - Your First FLEX Message

**From zero to first transmission in 30 minutes** 📡

This guide assumes you're completely new to FLEX paging and ESP32 devices. We'll walk you through everything from unboxing to sending your first message.

## 🎯 What You're About to Build

By the end of this guide, you'll have:
- A working FLEX paging message transmitter
- Sent your first pager message  
- Understanding of two control methods: **Web Interface** (easy) and **AT Commands** (advanced)

## 📦 What You Need

### Required Hardware
- **ESP32 LoRa32 development board** (one of these):
  - **TTGO LoRa32-OLED** ✅ **RECOMMENDED** (ESP32 + SX1276, beginner-friendly, WiFi web interface, 248 character support)
  - **Heltec WiFi LoRa 32 V2** ✅ **FULLY SUPPORTED** (ESP32 + SX1276, WiFi web interface, 248 character support)
- **USB cable** (USB-C, must support data transfer)
- **Antenna** (comes with most boards, or 915MHz/868MHz antenna)
- **Computer** with USB port (Windows, Mac, or Linux)

⚠️ **Hardware Compatibility Note**: Only Heltec WiFi LoRa 32 **V2** is supported (SX1276 chipset). Heltec V3 is **NOT SUPPORTED** due to SX1262 chipset incompatibility with FLEX protocol timing requirements.

### Software You'll Install
- **Arduino IDE** (free download)
- **Project code** (we'll download this)

### What You'll Learn
- How to set up the hardware
- Two ways to send messages: **Web Interface** (point and click) or **AT Commands** (terminal)

---

## 🚀 Step 1: Get Your Hardware Ready

### Unbox and Inspect Your Device
1. **Remove ESP32 board** from packaging
2. **Locate the antenna connector** (small gold connector)
3. **Attach the antenna** - gently connect antenna to the gold connector
   - ⚠️ **NEVER power on without antenna** - this can damage the radio
4. **Find the USB port** (usually USB-C)

### Connect to Computer
1. **Plug USB cable** into your ESP32 device
2. **Connect other end** to your computer
3. **Look for power indicators** - LED should light up on the board
4. **Check if computer detects device**:
   - **Windows**: Check Device Manager → Ports
   - **Mac/Linux**: Open terminal and run `ls /dev/tty*` - look for new device

---

## 🔧 Step 2: Install Required Software

### Download Arduino IDE
1. **Visit**: [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
2. **Download** Arduino IDE for your operating system
3. **Install** following the normal installation process
4. **Open** Arduino IDE

### Get the Project Code
1. **Download project**:
   ```bash
   # If you have git installed:
   git clone --recursive https://github.com/geekinsanemx/flex-fsk-tx.git
   
   # OR download ZIP from GitHub and extract
   ```
2. **Navigate to project folder**
3. **Note the location** - you'll need this for firmware installation

---

## 📱 Step 3: Choose Your Experience Level

### 🟢 **Beginner Path: Web Interface (Recommended)**
- ✅ **TTGO LoRa32-OLED device required**
- ✅ Control via web browser (no terminal needed)
- ✅ WiFi setup with visual interface
- ✅ Point-and-click message sending

### 🟡 **Advanced Path: AT Commands**
- ✅ Works with any ESP32 LoRa32 device
- ✅ Terminal/command line control
- ✅ Direct USB connection (no WiFi needed)
- ✅ More technical but very powerful

**Choose your path below** ⬇️

---

## 🌐 Path A: Web Interface (Beginner-Friendly)

*Requirements: TTGO LoRa32-OLED device*

### Step A1: Install Special Firmware

**This gives your device WiFi powers and a web interface**

1. **Install firmware and libraries**: Follow [FIRMWARE.md](FIRMWARE.md) for complete instructions
   - Choose **TTGO LoRa32-OLED v3 firmware**
   - This includes WiFi, web interface, and REST API
2. **Verify installation**: Device should show "flex-fsk-tx" on its small screen

### Step A2: First Power-On Setup

1. **Power on device** (plug into USB or battery)
2. **Wait 30 seconds** for full startup
3. **Check the small screen** - should display:
   ```
   AP Mode Active
   TTGO_FLEX_XXXX (or HELTEC_FLEX_XXXX)
   Pass: 12345678
   IP: 192.168.4.1
   ```

### Step A3: Connect to Device's WiFi

Your device creates its own WiFi network for setup:

1. **On your phone/computer**: Look for WiFi network with device-specific name:
   - TTGO devices: `TTGO_FLEX_XXXX` (4 hex characters)
   - Heltec devices: `HELTEC_FLEX_XXXX` (4 hex characters)
   - The XXXX will be unique to your device
2. **Connect to this network**:
   - **Password**: `12345678`
3. **Open web browser** and go to: `http://192.168.4.1`
4. **You should see**: FLEX Paging Message Transmitter configuration page

### Step A4: Connect Device to Your WiFi

1. **Click "Configuration"** on the webpage
2. **Enter your WiFi details**:
   - **Network Name (SSID)**: Your home/office WiFi name
   - **Password**: Your WiFi password
3. **Click "Save Settings"**
4. **Device will restart** and connect to your WiFi
5. **Note the new IP address** shown on device screen (e.g., `192.168.1.100`)

### Step A5: Send Your First Message!

1. **Open web browser** and go to the new IP address (e.g., `http://192.168.1.100`)
2. **Fill in the message form**:
   - **Frequency**: `929.6625` (common US frequency)
   - **Power**: `10` (safe starting power)
   - **Capcode**: `1234567` (test pager number)
   - **Message**: `Hello World - My First FLEX Message!`
3. **Click "Send Message"**
4. **Watch the device screen** - should show "Transmitting..."
5. **Success!** 🎉 You've sent your first FLEX message

### What Just Happened?
- Your device transmitted a FLEX paging signal
- Any pager set to capcode `1234567` would receive your message
- The signal went out at 929.6625 MHz frequency
- You used 10 dBm of transmission power

---

## 💻 Path B: AT Commands (Advanced Users)

*Works with any ESP32 LoRa32 device*

### Step B1: Install Basic Firmware

1. **Install firmware**: Follow [FIRMWARE.md](FIRMWARE.md) for complete instructions
   - For **TTGO**: Choose v1 or v2 firmware
   - For **Heltec**: Choose v1 or v2 firmware
2. **Verify installation**: Device should show "flex-fsk-tx" on screen

### Step B2: Build Command-Line Tool

1. **Open terminal** in project directory
2. **Build the software**:
   ```bash
   make
   sudo make install
   ```
3. **Verify installation**:
   ```bash
   flex-fsk-tx --help
   ```

### Step B3: Find Your Device

1. **Identify serial port**:
   ```bash
   # Linux/Mac:
   ls /dev/tty*
   # Look for /dev/ttyUSB0 (Heltec) or /dev/ttyACM0 (TTGO)
   
   # Windows: Check Device Manager → Ports
   ```

### Step B4: Test Communication

1. **Test basic connection**:
   ```bash
   # Linux/Mac example:
   echo "AT" | screen /dev/ttyUSB0 115200
   # Should respond: OK
   ```

### Step B5: Send Your First Message!

**Simple method** (works with all firmware):
```bash
# Replace /dev/ttyUSB0 with your device port
flex-fsk-tx -d /dev/ttyUSB0 1234567 "Hello World - My First FLEX Message!"
```

**Advanced method** (v2+ firmware with on-device encoding):
```bash
# Uses device's built-in FLEX encoder
flex-fsk-tx -d /dev/ttyUSB0 -r 1234567 "Hello World - My First FLEX Message!"
```

**Manual AT commands** (for learning):
```bash
# Connect to device
screen /dev/ttyUSB0 115200

# Send commands:
AT+FREQ=929.6625    # Set frequency
AT+POWER=10         # Set power
AT+MSG=1234567      # Start message to capcode 1234567
Hello World!        # Type message and press Enter
# Device responds: OK (message sent!)
```

---

## 🎯 Understanding What You Just Did

### FLEX Paging Basics
- **FLEX** = A digital paging protocol
- **Capcode** = Unique number identifying each pager (like a phone number)
- **Frequency** = Radio frequency for transmission (must match pager's frequency)
- **Power** = Transmission strength (higher = further range)

### Your Message Journey
1. **You typed** a message and capcode
2. **Device encoded** message in FLEX format
3. **Radio transmitted** signal at specified frequency
4. **Any pager** with matching capcode received the message

---

## 🔧 Control Methods Comparison

| Feature | Web Interface | AT Commands |
|---------|---------------|-------------|
| **Ease of Use** | ⭐⭐⭐⭐⭐ Very Easy | ⭐⭐⭐ Moderate |
| **Setup Time** | 10 minutes | 15 minutes |
| **Device Required** | TTGO only | Any ESP32 LoRa32 |
| **Network Needed** | WiFi | USB cable only |
| **Best For** | Beginners, occasional use | Advanced users, automation |
| **Features** | Basic transmission | Full device control |

---

## 🎛️ Next Steps - Explore More Features

### Web Interface Users
1. **Try different frequencies** - 915.0, 931.9375 MHz
2. **Adjust power levels** - Start low (5 dBm), test range
3. **Enable Mail Drop** - Makes message urgent/priority
4. **Explore Configuration** - Custom device name, API settings
5. **Use REST API** - Send messages from other programs

### AT Command Users
1. **Learn more commands** - See [AT_COMMANDS.md](AT_COMMANDS.md) for complete reference
2. **Automate messaging** - Write scripts for bulk messages
3. **Try remote encoding** - Use `-r` flag for on-device FLEX encoding
4. **Loop mode** - Send multiple messages from file input

---

## 🚨 Common First-Time Issues

### "Device not found"
- **Check USB cable** - must support data (not just charging)
- **Try different port** - /dev/ttyUSB1, COM4, etc.
- **Install drivers** - Some devices need specific USB drivers

### "Firmware upload failed"
- **Hold BOOT button** while clicking Upload in Arduino IDE
- **Try slower speed** - Change upload speed to 115200
- **Check board selection** - Must match your hardware exactly

### "Web interface not working"
- **Check device screen** - should show IP address
- **Verify same network** - Computer and device on same WiFi
- **Try direct connection** - Connect to device's AP mode

### "No response to AT commands"
- **Check baud rate** - Must be 115200
- **Set line endings** - Use "Both NL & CR" in terminal
- **Press RESET** - Reset device and try again

---

## 📚 Where to Go From Here

### Complete Documentation
- **[USER_GUIDE.md](USER_GUIDE.md)** - Complete web interface guide
- **[AT_COMMANDS.md](AT_COMMANDS.md)** - Full AT command reference
- **[REST_API.md](REST_API.md)** - API for developers
- **[FIRMWARE.md](FIRMWARE.md)** - Detailed firmware installation
- **[README.md](../README.md)** - Complete project overview

### Learning More
- **Frequency Selection** - Research legal frequencies in your region
- **Antenna Optimization** - Better antennas = better range
- **Power Regulations** - Check legal power limits for your area
- **FLEX Protocol** - Learn about message types and encoding

### Community and Support
- **Project Issues** - Report problems on GitHub
- **Ham Radio Forums** - Discuss with other users
- **Arduino Community** - ESP32 development help

---

## ⚠️ Important Safety Notes

### Legal Compliance
- **Check local regulations** - Verify frequency and power limits in your region
- **Amateur radio license** - May be required for some frequencies
- **Interference** - Avoid interfering with existing services

### Hardware Safety
- **Always use antenna** - Never transmit without proper antenna connected
- **Power limits** - Start with low power (5-10 dBm) and increase gradually
- **Heat management** - Device may get warm during transmission

### Best Practices
- **Test thoroughly** - Verify range and clarity before important use
- **Document settings** - Keep record of working frequencies and power levels
- **Regular updates** - Check for firmware and software updates

---

## 🎉 Congratulations!

You've successfully:
- ✅ Set up your FLEX paging message transmitter hardware
- ✅ Installed and configured the firmware
- ✅ Sent your first FLEX paging message
- ✅ Learned two different control methods

**You're now ready to explore the full capabilities of your FLEX paging message transmitter!**

Whether you choose the user-friendly web interface or the powerful AT command system, you have a solid foundation to build upon. Happy paging! 📡

---

## 🆘 Need Help?

**🔧 Issues Not Covered Here?** See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive problem resolution including:
- Complete hardware troubleshooting procedures
- Firmware installation and upload issues
- Advanced AT command and network problems
- Emergency recovery procedures
- Professional GitHub issue reporting

**Quick Solutions**:
1. **Re-read this guide** - Most common issues are covered above
2. **Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Comprehensive issue resolution
3. **Try different approach** - Web interface vs. AT commands
4. **Start fresh** - Factory reset device and begin again

**Emergency Reset**:
- **AT Command**: Send `AT+FACTORYRESET` via serial
- **Hardware Reset**: Hold BOOT button for 30 seconds
- **Factory Settings**: Device returns to AP mode with default settings

**Still Need Help?** Follow the GitHub issue reporting process in [TROUBLESHOOTING.md](TROUBLESHOOTING.md).