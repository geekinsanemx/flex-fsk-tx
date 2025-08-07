# ESP32 Hardware and Firmware Setup Guide

This guide covers the hardware requirements and firmware installation for the flex-fsk-tx system's hardware transmitter layer.

## Hardware Requirements

### Supported Hardware

**Heltec WiFi LoRa 32 V3**
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
- **Serial Port**: Typically appears as `/dev/ttyUSB0` on Linux
- **Firmware Location**: `Devices/Heltec LoRa32 V3/heltec_fsk_tx_AT.ino`
- **Dimensions**: 25.5 × 51 × 13.5 mm

**TTGO LoRa32-OLED**
- **Manufacturer**: LilyGO (TTGO)
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio Chipset**: Semtech SX1276 LoRa/FSK transceiver
- **Frequency Bands**:
  - 433 MHz / 868 MHz / 915 MHz (region dependent)
- **TX Power**: Up to +20 dBm (100 mW)
- **Display**: 0.96" OLED (128x64 pixels, SSD1306)
- **Connectivity**: USB-C, WiFi 802.11 b/g/n, Bluetooth
- **Power**: USB-C or Li-Po battery connector
- **Serial Port**: Typically appears as `/dev/ttyACM0` on Linux
- **Firmware Location**: `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT.ino`
- **Dimensions**: ~51 × 25.5 × 13 mm

### Alternative Compatible Hardware

**Heltec Wireless Stick V3** - *Compatible with Pin Adjustments*
- Similar ESP32-S3 + SX1262 architecture
- Smaller form factor without full OLED display
- May require firmware modifications for display support

**Other SX1262/SX1276-based ESP32 Devices** - *May Require Pin Configuration Changes*
- Custom ESP32 + SX1262/SX1276 boards with compatible pin mapping
- Pin definitions may need adjustment in firmware

### Required Accessories

1. **Antenna**:
   - Frequency-appropriate antenna (902-928 MHz for US, 863-870 MHz for EU)
   - SMA or u.FL connector depending on board variant
   - **Warning**: Never operate without antenna - can damage the radio chipset

2. **Connection Cable**:
   - USB-C cable for both Heltec V3 and TTGO LoRa32-OLED
   - USB-A to USB-C cable for computer connection

3. **Computer Requirements**:
   - Linux/Unix system (primary support)
   - Windows with WSL (alternative)
   - macOS (community support)
   - USB port and serial driver support

### Hardware Procurement

**Heltec WiFi LoRa 32 V3**:
- [Heltec Official Store](https://heltec.org/proudct_center/lora/)
- [Heltec Automation Website](https://heltec.org/)
- Authorized Distributors: Digi-Key, Mouser, Arrow, Newark/Farnell
- Online Marketplaces: Amazon, AliExpress (verify seller authenticity)

**TTGO LoRa32-OLED**:
- LilyGO official stores on AliExpress
- Electronics retailers: Banggood, Amazon, eBay
- **Note**: Ensure you get the version with OLED display for full functionality
- **Important**: Verify it's the SX1276-based variant (most common)

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
   - **For Heltec WiFi LoRa 32 V3**:
     - Go to `Tools` > `Board` > `ESP32 Arduino`
     - Select **"Heltec WiFi LoRa 32(V3)"**
   - **For TTGO LoRa32-OLED**:
     - Go to `Tools` > `Board` > `ESP32 Arduino`
     - Select **"TTGO LoRa32-OLED"** or **"ESP32 Dev Module"**

### Required Libraries

Install the following libraries through Arduino IDE Library Manager (`Tools` > `Manage Libraries`):

#### Core Libraries
1. **RadioLib** by Jan Gromeš
   - Version: Latest (tested with v6.x)
   - Search: "RadioLib"
   - Install: "RadioLib by Jan Gromes"
   - **Note**: Required for both SX1262 (Heltec V3) and SX1276 (TTGO) support

2. **Heltec ESP32 Library** (for Heltec devices)
   - Version: Latest
   - Search: "Heltec ESP32"
   - Install: "Heltec ESP32 Dev-Boards by Heltec Automation"

3. **U8g2** (for TTGO devices with manual OLED setup)
   - Version: Latest
   - Search: "U8g2"
   - Install: "U8g2 by oliver"
   - **Note**: May be needed for TTGO devices depending on firmware implementation

#### Alternative Installation Method
If you prefer manual installation or encounter issues:

```bash
# Clone libraries to your Arduino libraries directory
cd ~/Arduino/libraries/

# RadioLib (supports both SX1262 and SX1276)
git clone https://github.com/jgromes/RadioLib.git

# Heltec ESP32 library (for Heltec devices)
git clone https://github.com/HelTecAutomation/Heltec_ESP32.git

# U8g2 for OLED displays (for TTGO devices)
git clone https://github.com/olikraus/u8g2.git
```

## Hardware Pin Configuration

### Heltec WiFi LoRa 32 V3 (SX1262)

The firmware uses the following pin configuration optimized for **Heltec WiFi LoRa 32 V3**:

#### SX1262 Radio Interface
```cpp
#define LORA_NSS    8   // SPI Chip Select (CS)
#define LORA_NRESET 12  // Radio Reset pin  
#define LORA_BUSY   13  // Radio Busy status
#define LORA_DIO1   14  // Digital I/O 1 (IRQ)
#define LORA_SCK    9   // SPI Clock
#define LORA_MISO   11  // SPI Master In Slave Out
#define LORA_MOSI   10  // SPI Master Out Slave In
```

#### System Control Pins
```cpp
#define LED_PIN     35  // Built-in LED (active HIGH)
// OLED Display uses I2C (SDA_OLED, SCL_OLED - auto-configured)
// Vext power control - auto-configured by Heltec library
```

### TTGO LoRa32-OLED (SX1276)

The TTGO device uses different pin configurations and is handled by the dedicated firmware at `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT.ino`:

#### SX1276 Radio Interface (typical)
```cpp
#define LORA_NSS    18  // SPI Chip Select (CS)
#define LORA_NRESET 14  // Radio Reset pin  
#define LORA_DIO0   26  // Digital I/O 0 (IRQ)
#define LORA_SCK    5   // SPI Clock
#define LORA_MISO   19  // SPI Master In Slave Out
#define LORA_MOSI   27  // SPI Master Out Slave In
```

#### System Control Pins (typical)
```cpp
#define LED_PIN     25  // Built-in LED
// OLED Display typically on I2C pins 4 (SDA) and 15 (SCL)
```

**Note**: Pin configurations may vary between TTGO board revisions. The firmware at `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT.ino` is specifically configured for the most common TTGO LoRa32-OLED variant.

### Hardware Validation

Before flashing, verify your board has:
- ✅ ESP32 or ESP32-S3 microcontroller
- ✅ SX1262 (Heltec V3) or SX1276 (TTGO) radio chipset
- ✅ Proper SPI connections between MCU and radio
- ✅ Working USB-C connector and power management
- ✅ I2C OLED display (recommended for status monitoring)

## Flashing Process

### Step 1: Prepare the Hardware

1. **Hardware Inspection**:
   - Verify you have the correct device (Heltec V3 or TTGO LoRa32-OLED)
   - Check for physical damage, especially to the radio module
   - Ensure the USB-C connector is intact

2. **Antenna Connection**:
   - **CRITICAL**: Connect an appropriate antenna before powering on
   - Use 902-928 MHz antenna for US/Canada
   - Use 863-870 MHz antenna for Europe
   - **Never operate without antenna** - can permanently damage the radio chipset

3. **USB Connection**:
   - Connect the ESP32 to your computer via USB-C cable
   - Device should appear as a serial port:
     - **Heltec V3**: Usually `/dev/ttyUSB0`, `/dev/ttyUSB1`, etc.
     - **TTGO LoRa32-OLED**: Usually `/dev/ttyACM0`, `/dev/ttyACM1`, etc.
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

#### For Heltec WiFi LoRa 32 V3:

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

#### For TTGO LoRa32-OLED:

1. **Select the correct board**:
   - `Tools` > `Board` > `ESP32 Arduino` > `TTGO LoRa32-OLED V1` (or `ESP32 Dev Module`)

2. **Set the correct port**:
   - `Tools` > `Port` > `/dev/ttyACM0` (or your device's port)

3. **Configure upload settings**:
   - **Upload Speed**: `115200` or `921600`
   - **CPU Frequency**: `240MHz (WiFi/BT)`
   - **Flash Frequency**: `80MHz`
   - **Flash Mode**: `QIO`
   - **Flash Size**: `4MB (32Mb)` (typical for TTGO)
   - **Partition Scheme**: `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)`
   - **Core Debug Level**: `None`

### Step 3: Upload the Firmware

#### For Heltec WiFi LoRa 32 V3:

1. **Open the firmware file**:
   - `File` > `Open` > Navigate to `RadioTransmitter_AT.ino` (main firmware)

2. **Verify compilation**:
   - Click the checkmark (✓) button to verify the code compiles without errors

3. **Upload the firmware**:
   - Click the arrow (→) button to upload
   - **Put the device in download mode** if required:
     - Hold the `BOOT` button
     - Press and release the `RESET` button
     - Release the `BOOT` button
   - The upload process should complete in 1-2 minutes

#### For TTGO LoRa32-OLED:

1. **Open the firmware file**:
   - `File` > `Open` > Navigate to `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT.ino`

2. **Verify compilation**:
   - Click the checkmark (✓) button to verify the code compiles without errors
   - Ensure all required libraries are installed (RadioLib, U8g2)

3. **Upload the firmware**:
   - Click the arrow (→) button to upload
   - **Put the device in download mode** if required:
     - Hold the `BOOT` button (if present)
     - Press and release the `RESET` button
     - Release the `BOOT` button
   - The upload process should complete in 1-2 minutes

### Step 4: Verify Installation

#### Common Verification Steps:

1. **Initial Power-On Test**:
   - Press the `RESET` button on the ESP32
   - OLED display should show:
     - **Banner**: Device identification at the top
     - **Status**: "Ready"
     - **TX Power**: "2.0 dBm" (or configured default)
     - **Frequency**: Default frequency (varies by device)

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
   +FREQ: 931.9375            ← Expected response (may vary by device)
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

#### Device-Specific Verification:

**Heltec WiFi LoRa 32 V3 Specific**:
- Default frequency should be around 931.9375 MHz
- Serial port typically `/dev/ttyUSB0`
- Banner should show "GeekInsaneMX" or similar
- SX1262 radio capabilities (higher frequency range)

**TTGO LoRa32-OLED Specific**:
- Default frequency may vary (typically 915.0 MHz)
- Serial port typically `/dev/ttyACM0`
- Banner should show "ttgo-fsk-tx" or similar
- SX1276 radio capabilities (traditional LoRa frequencies)

## Post-Flash Configuration

### System Integration Test

Once firmware verification is complete, test integration with the host application:

1. **Close Arduino Serial Monitor** (important - only one program can access the serial port)

2. **Test with Host Application**:
   ```bash
   # Build the host application first (see main README.md)
   cd /path/to/flex-fsk-tx
   make

   # Test basic communication - Heltec V3
   echo "1234567:Test Message" | ./bin/flex-fsk-tx -d /dev/ttyUSB0 -

   # Test basic communication - TTGO LoRa32-OLED
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
   - Default frequencies vary by device and region
   - US: 902-928 MHz ISM band
   - EU: 863-870 MHz SRD band

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
- Verify correct port selection (`/dev/ttyUSB0` for Heltec, `/dev/ttyACM0` for TTGO)
- Try manual boot mode:
  1. Hold `BOOT` button (if present)
  2. Press `RESET` button
  3. Release `RESET` button
  4. Release `BOOT` button
  5. Try upload again
- Try lower upload speed (115200)
- Check board selection matches your hardware

#### 2. Compilation Errors

**Symptoms**: "Library not found" or compilation failures

**Solutions**:
- Verify all required libraries are installed:
  - RadioLib (essential for both devices)
  - Heltec ESP32 library (for Heltec devices)
  - U8g2 (may be needed for TTGO devices)
- Check board selection matches your hardware
- Update Arduino IDE and libraries to latest versions
- Clear Arduino cache: `File` > `Preferences` > Delete contents of temp directory

#### 3. Device Not Responding After Flash

**Symptoms**: No "AT READY" message, no response to AT commands

**Solutions**:
- Press `RESET` button on device
- Check serial connection and baud rate (115200)
- Verify correct port selection:
  - `/dev/ttyUSB0` for Heltec WiFi LoRa 32 V3
  - `/dev/ttyACM0` for TTGO LoRa32-OLED
- Check if device entered deep sleep - press `RESET`
- Try different USB cable

#### 4. Display Not Working

**Symptoms**: OLED display remains blank

**Solutions**:
- Verify correct libraries are installed for your device
- Check board selection (must match hardware version)
- For TTGO devices: ensure U8g2 library is properly installed
- Try pressing `RESET` button
- Check I2C connections if using custom hardware

#### 5. Radio Not Transmitting

**Symptoms**: Commands accepted but no RF output

**Solutions**:
- Check antenna connection (CRITICAL)
- Verify frequency is within your region's legal limits
- Test with lower power settings first
- Check for RadioLib compilation errors
- Ensure correct radio chipset (SX1262 vs SX1276) firmware match

#### 6. Wrong Serial Port

**Symptoms**: Permission denied, device not found

**Solutions**:
- List available ports: `ls /dev/tty*`
- Check dmesg when plugging device: `dmesg | tail`
- Try different port:
  - `/dev/ttyUSB0`, `/dev/ttyUSB1` for Heltec
  - `/dev/ttyACM0`, `/dev/ttyACM1` for TTGO
- Add user to dialout group: `sudo usermod -a -G dialout $USER`

### Debug Mode

To enable detailed debug output, look for debug defines in the firmware and uncomment them:
```cpp
// Uncomment for debug messages
// #define DEBUG_SERIAL 1
```

Then recompile and flash. Debug messages will appear in the serial monitor.

### Factory Reset

To reset the device to default settings:
1. Send `AT+RESET` command, or
2. Press and hold `BOOT` button for 10 seconds, then release (if available)

## Device-Specific Firmware Features

### Heltec WiFi LoRa 32 V3 Features

- **AT Commands Supported**: Full AT command set
- **Radio**: SX1262 with extended frequency range (410-1000 MHz)
- **Power Range**: -9 to +22 dBm
- **Display**: Automatic Heltec OLED integration
- **Power Management**: Heltec Vext control

### TTGO LoRa32-OLED Features

- **AT Commands Supported**: Full AT command set
- **Radio**: SX1276 with traditional LoRa frequencies
- **Power Range**: Typically -4 to +20 dBm
- **Display**: Manual U8g2 OLED integration
- **Firmware Location**: `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT.ino`

### Common Features (Both Devices)

- **Power Management**: Display timeout after 5 minutes of inactivity
- **Wake on Activity**: Any AT command or transmission wakes the display
- **Status Indicators**: Built-in LED illuminates during transmission
- **Serial Protocol**: Standardized AT command interface at 115200 baud

## Performance Specifications

- **Transmission Mode**: FSK (Frequency Shift Keying)
- **Bit Rate**: 1.6 kbps
- **Frequency Deviation**: 5 kHz
- **RX Bandwidth**: 10.4 kHz
- **Maximum Data Size**: 2048 bytes per transmission
- **Serial Baudrate**: 115200 bps
- **Command Timeout**: 8 seconds
- **Data Timeout**: 15 seconds
- **Frequency Range**:
  - Heltec V3 (SX1262): 400-1000 MHz
  - TTGO (SX1276): Device dependent, typically 433/868/915 MHz

## Next Steps

Once the firmware is successfully flashed and verified:

1. **Return to main documentation** for host application setup
2. **Build and install** the flex-fsk-tx host application
3. **Test complete system** with device-appropriate serial port:
   - Heltec V3: `/dev/ttyUSB0`
   - TTGO LoRa32-OLED: `/dev/ttyACM0`

For complete system usage, refer to the main [README.md](README.md) file.
