# ESP32 Hardware and Firmware Setup Guide

This guide covers the hardware requirements and firmware installation for the flex-fsk-tx system's hardware transmitter layer.

## Hardware Requirements

### Primary Supported Hardware

**Heltec WiFi LoRa 32 V3** - *Recommended and Fully Tested*
- **Product Page**: [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/)
- **Manufacturer**: [Heltec Automation](https://heltec.org/)
- **MCU**: ESP32-S3 (240MHz dual-core Xtensa LX7)
- **Radio Chipset**: Semtech SX1262 LoRa/FSK transceiver
- **Frequency Bands**:
  - 410-525 MHz (sub-GHz band)
  - 863-928 MHz (ISM band)
- **TX Power**: Up to +22 dBm (158 mW)
- **Display**: 0.96" OLED (128x64 pixels, SSD1306)
- **Connectivity**: USB-C, WiFi 802.11 b/g/n, Bluetooth 5.0
- **Power**: USB-C or Li-Po battery connector
- **Dimensions**: 25.5 × 51 × 13.5 mm

### Alternative Compatible Hardware

**Heltec Wireless Stick V3** - *Compatible with Pin Adjustments*
- Similar ESP32-S3 + SX1262 architecture
- Smaller form factor without full OLED display
- May require firmware modifications for display support

**Other SX1262-based Heltec Devices** - *May Require Pin Configuration Changes*
- Heltec WiFi LoRa 32 V2 (older ESP32 + SX1276 - **NOT recommended**)
- Custom ESP32 + SX1262 boards with compatible pin mapping

### Required Accessories

1. **Antenna**:
   - Frequency-appropriate antenna (902-928 MHz for US, 863-870 MHz for EU)
   - SMA or u.FL connector depending on board variant
   - **Warning**: Never operate without antenna - can damage the radio chipset

2. **Connection Cable**:
   - USB-C cable for Heltec V3 boards
   - USB-A to USB-C cable for computer connection

3. **Computer Requirements**:
   - Linux/Unix system (primary support)
   - Windows with WSL (alternative)
   - macOS (community support)
   - USB port and serial driver support

### Hardware Procurement

**Official Sources**:
- [Heltec Official Store](https://heltec.org/proudct_center/lora/)
- [Heltec Automation Website](https://heltec.org/)

**Authorized Distributors**:
- Digi-Key Electronics
- Mouser Electronics  
- Arrow Electronics
- Newark/Farnell

**Online Marketplaces**:
- Amazon (verify seller authenticity)
- AliExpress (official Heltec store)
- eBay (buy from reputable sellers)

**Pricing**: Typically $15-30 USD depending on source and region

## Prerequisites

### Arduino IDE Setup

1. **Install Arduino IDE**
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install arduino

   # Or download from https://www.arduino.cc/en/software
   ```

2. **Add ESP32 Board Manager**
   - Open Arduino IDE
   - Go to `File` > `Preferences`
   - Add this URL to "Additional Boards Manager URLs":
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Go to `Tools` > `Board` > `Boards Manager`
   - Search for "ESP32" and install "esp32 by Espressif Systems"

3. **Select Board Configuration**
   - Go to `Tools` > `Board` > `ESP32 Arduino`
   - Select **"Heltec WiFi LoRa 32(V3)"** for Heltec V3 boards
   - Or select **"Heltec Wireless Stick(V3)"** for Wireless Stick V3

### Required Libraries

Install the following libraries through Arduino IDE Library Manager (`Tools` > `Manage Libraries`):

#### Core Libraries
1. **RadioLib** by Jan Gromeš
   - Version: Latest (tested with v6.x)
   - Search: "RadioLib"
   - Install: "RadioLib by Jan Gromes"

2. **Heltec ESP32 Library**
   - Version: Latest
   - Search: "Heltec ESP32"
   - Install: "Heltec ESP32 Dev-Boards by Heltec Automation"

#### Alternative Installation Method
If you prefer manual installation or encounter issues:

```bash
# Clone libraries to your Arduino libraries directory
cd ~/Arduino/libraries/

# RadioLib
git clone https://github.com/jgromes/RadioLib.git

# Heltec ESP32 library
git clone https://github.com/HelTecAutomation/Heltec_ESP32.git
```

## Hardware Pin Configuration

The firmware uses the following pin configuration optimized for **Heltec WiFi LoRa 32 V3**:

### SX1262 Radio Interface
```cpp
#define LORA_NSS    8   // SPI Chip Select (CS)
#define LORA_NRESET 12  // Radio Reset pin  
#define LORA_BUSY   13  // Radio Busy status
#define LORA_DIO1   14  // Digital I/O 1 (IRQ)
#define LORA_SCK    9   // SPI Clock
#define LORA_MISO   11  // SPI Master In Slave Out
#define LORA_MOSI   10  // SPI Master Out Slave In
```

### System Control Pins
```cpp
#define LED_PIN     35  // Built-in LED (active HIGH)
// OLED Display uses I2C (SDA_OLED, SCL_OLED - auto-configured)
// Vext power control - auto-configured by Heltec library
```

### Pin Configuration for Other Boards

**For Heltec Wireless Stick V3**, uncomment this line in the firmware:
```cpp
#define WIRELESS_STICK_V3  // Enable for Wireless Stick V3
```

**For custom ESP32 + SX1262 boards**, modify the pin definitions according to your hardware schematic. Ensure proper SPI connections and interrupt handling.

### Hardware Validation

Before flashing, verify your board has:
- ✅ ESP32-S3 microcontroller (ESP32 variants may work but are not officially supported)
- ✅ SX1262 radio chipset (SX1276 boards will **NOT** work)
- ✅ Proper SPI connections between MCU and radio
- ✅ Working USB-C connector and power management
- ✅ I2C OLED display (optional but recommended)

## Flashing Process

### Step 1: Prepare the Hardware

1. **Hardware Inspection**:
   - Verify you have a genuine Heltec WiFi LoRa 32 V3 board
   - Check for physical damage, especially to the SX1262 radio module
   - Ensure the USB-C connector is intact

2. **Antenna Connection**:
   - **CRITICAL**: Connect an appropriate antenna before powering on
   - Use 902-928 MHz antenna for US/Canada
   - Use 863-870 MHz antenna for Europe
   - **Never operate without antenna** - can permanently damage the SX1262

3. **USB Connection**:
   - Connect the ESP32 to your computer via USB-C cable
   - Device should appear as `/dev/ttyUSB0`, `/dev/ttyACM0`, or similar
   - Windows: Check Device Manager for new COM port
   - macOS: Check `/dev/cu.usbserial-*` or `/dev/cu.usbmodem-*`

4. **Driver Installation** (if needed):
   ```bash
   # Linux - usually automatic, but if needed:
   sudo apt install ch341-uart-dkms  # For CH340/CH341 USB chips

   # Add user to dialout group
   sudo usermod -a -G dialout $USER
   # Log out and back in, or run:
   newgrp dialout
   ```

5. **Hardware Test**:
   - Power LED should illuminate when connected
   - OLED display may show boot messages or remain blank (normal)
   - No smoke, unusual heat, or burning smells

### Step 2: Configure Arduino IDE

1. **Select the correct board**:
   - `Tools` > `Board` > `ESP32 Arduino` > `Heltec WiFi LoRa 32(V3)`

2. **Set the correct port**:
   - `Tools` > `Port` > `/dev/ttyUSB0` (or your device's port)

3. **Configure upload settings**:
   - **Upload Speed**: `115200` or `921600`
   - **CPU Frequency**: `240MHz (WiFi/BT)`
   - **Flash Frequency**: `80MHz`
   - **Flash Mode**: `QIO`
   - **Flash Size**: `8MB (64Mb)`
   - **Partition Scheme**: `8M Flash (3MB APP/1.5MB SPIFFS)`
   - **Core Debug Level**: `None`

### Step 3: Upload the Firmware

1. **Open the firmware file**:
   - `File` > `Open` > Navigate to `RadioTransmitter_AT.ino`

2. **Verify compilation**:
   - Click the checkmark (✓) button to verify the code compiles without errors

3. **Upload the firmware**:
   - Click the arrow (→) button to upload
   - **Put the device in download mode** if required:
     - Hold the `BOOT` button
     - Press and release the `RESET` button
     - Release the `BOOT` button
   - The upload process should complete in 1-2 minutes

### Step 4: Verify Installation

1. **Initial Power-On Test**:
   - Press the `RESET` button on the ESP32
   - OLED display should show:
     - **Banner**: "GeekInsaneMX" at the top
     - **Status**: "Ready"
     - **TX Power**: "2.0 dBm"
     - **Frequency**: "931.9375 MHz"

2. **Serial Communication Test**:
   - Open Serial Monitor: `Tools` > `Serial Monitor`
   - Set baud rate to `115200`
   - Ensure line ending is set to `Both NL & CR`

3. **AT Command Verification**:
   ```
   AT READY                    ← Should appear automatically on reset

   AT                          ← Type this and press Enter
   OK                          ← Expected response

   AT+FREQ?                    ← Query frequency
   +FREQ: 931.9375            ← Expected response
   OK

   AT+POWER?                   ← Query power
   +POWER: 2                  ← Expected response  
   OK

   AT+STATUS?                  ← Query status
   +STATUS: READY             ← Expected response
   OK
   ```

4. **Hardware Function Test**:
   - LED should be OFF when idle
   - OLED display should be active and readable
   - No error messages in serial output

5. **Radio Test** (Optional):
   ```
   AT+FREQ=916.0              ← Set test frequency
   OK

   AT+POWER=5                 ← Set low power for testing
   OK

   AT+SEND=10                 ← Prepare to send 10 bytes
   +SEND: READY               ← Device ready for data

   1234567890                 ← Send test data (exactly 10 bytes)
   OK                         ← Transmission completed
   ```
   - LED should briefly illuminate during transmission
   - Display should show "Transmitting..." during the process

## Post-Flash Configuration

### System Integration Test

Once firmware verification is complete, test integration with the host application:

1. **Close Arduino Serial Monitor** (important - only one program can access the serial port)

2. **Test with Host Application**:
   ```bash
   # Build the host application first (see main README.md)
   cd /path/to/flex-fsk-tx
   make

   # Test basic communication
   echo "1234567:Test Message" | ./bin/flex-fsk-tx -d /dev/ttyACM0 -
   ```

3. **Expected Output**:
   ```
   Testing device communication...
   Communication attempt 1/10...
   Sending: AT
   Received: 'OK'
   Device communication established
   Device communication confirmed stable

   Configuring radio parameters...
   Sending: AT+FREQ=916.0000
   Received: 'OK'
   Sending: AT+POWER=2
   Received: 'OK'
   Radio configured successfully.

   Attempting to send data (attempt 1/3)...
   [... transmission details ...]
   Transmission completed successfully!
   Successfully sent flex message
   ```

### Legal and Regulatory Compliance

**IMPORTANT**: Before using this system for transmission:

1. **Frequency Allocation**:
   - Verify the frequency is legal in your jurisdiction
   - Default 931.9375 MHz is within US ISM band (902-928 MHz)
   - EU users should configure for 863-870 MHz band

2. **Power Limits**:
   - Respect local power limitations
   - US: Up to 1W (30 dBm) in 902-928 MHz band
   - EU: Typically 25mW (14 dBm) in 863-870 MHz band

3. **Licensing**:
   - Some jurisdictions require amateur radio license
   - Commercial use may require different licensing
   - Consult local regulations before operation

4. **Interference**:
   - Monitor for interference to other services
   - Use minimum necessary power
   - Implement proper duty cycle limitations

### Configuration Commands for Different Regions

```bash
# US/Canada Configuration (902-928 MHz)
AT+FREQ=915.0    # Center of US ISM band
AT+POWER=20      # Up to 20 dBm (100mW) typical

# Europe Configuration (863-870 MHz)  
AT+FREQ=868.0    # Center of EU SRD band
AT+POWER=10      # 10 dBm (10mW) typical limit

# Custom Configuration
AT+FREQ=<your_freq>    # Your legal frequency
AT+POWER=<your_power>  # Your legal power limit
```

## Troubleshooting

### Common Issues

#### 1. Upload Failed / Cannot Connect

**Symptoms**: Upload fails, "Failed to connect" errors

**Solutions**:
- Check USB cable (try a different cable)
- Verify correct port selection
- Try manual boot mode:
  1. Hold `BOOT` button
  2. Press `RESET` button
  3. Release `RESET` button
  4. Release `BOOT` button
  5. Try upload again
- Try lower upload speed (115200)

#### 2. Compilation Errors

**Symptoms**: "Library not found" or compilation failures

**Solutions**:
- Verify all required libraries are installed
- Check board selection matches your hardware
- Update Arduino IDE and libraries to latest versions
- Clear Arduino cache: `File` > `Preferences` > Delete contents of temp directory

#### 3. Device Not Responding After Flash

**Symptoms**: No "AT READY" message, no response to AT commands

**Solutions**:
- Press `RESET` button on device
- Check serial connection and baud rate (115200)
- Verify correct port selection
- Check if device entered deep sleep - press `RESET`

#### 4. Display Not Working

**Symptoms**: OLED display remains blank

**Solutions**:
- Verify Heltec ESP32 library is installed
- Check board selection (must match hardware version)
- Some boards require `#define WIRELESS_STICK_V3` uncommented
- Try pressing `RESET` button

#### 5. Radio Not Transmitting

**Symptoms**: Commands accepted but no RF output

**Solutions**:
- Check antenna connection
- Verify frequency is within your region's legal limits
- Test with lower power settings first
- Check for RadioLib compilation errors

### Debug Mode

To enable detailed debug output, uncomment this line in the firmware:
```cpp
// Uncomment for debug messages
// #define DEBUG_SERIAL 1
```

Then recompile and flash. Debug messages will appear in the serial monitor.

### Factory Reset

To reset the device to default settings:
1. Send `AT+RESET` command, or
2. Press and hold `BOOT` button for 10 seconds, then release

## Firmware Features

### AT Commands Supported

- `AT` - Basic connectivity test
- `AT+FREQ=<MHz>` / `AT+FREQ?` - Set/query frequency (400-1000 MHz)
- `AT+POWER=<dBm>` / `AT+POWER?` - Set/query power (-9 to 22 dBm)
- `AT+SEND=<bytes>` - Initiate binary data transmission
- `AT+STATUS?` - Query device status
- `AT+ABORT` - Abort current operation
- `AT+RESET` - Software reset

### Power Management

- **Display Timeout**: OLED turns off after 5 minutes of inactivity
- **Wake on Activity**: Any AT command or transmission wakes the display
- **VEXT Control**: Automatic power management for Heltec boards

### Status Indicators

- **Built-in LED**: Illuminates during transmission
- **OLED Display**: Shows real-time status, frequency, and power settings
- **Serial Messages**: Comprehensive logging and debug information

## Performance Specifications

- **Transmission Mode**: FSK (Frequency Shift Keying)
- **Bit Rate**: 1.6 kbps
- **Frequency Deviation**: 5 kHz
- **RX Bandwidth**: 10.4 kHz
- **Maximum Data Size**: 2048 bytes per transmission
- **Serial Baudrate**: 115200 bps
- **Command Timeout**: 8 seconds
- **Data Timeout**: 15 seconds

## Next Steps

Once the firmware is successfully flashed and verified:

1. **Return to main documentation** for host application setup
2. **Build and install** the flex-fsk-tx host application
3. **Test complete system** with a simple message transmission

For complete system usage, refer to the main [README.md](README.md) file.
