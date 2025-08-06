# ESP32 Firmware Flashing Guide

This guide covers the installation and flashing of the `RadioTransmitter_AT.ino` firmware for ESP32 LoRa32 devices.

## Hardware Compatibility

The firmware has been tested and optimized for:
- **Heltec WiFi LoRa 32 V3** (Primary target)
- **Heltec Wireless Stick V3** (Alternative configuration)
- Other ESP32 LoRa32 devices with SX1262 radio (may require pin adjustments)

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

The firmware uses the following pin configuration for **Heltec WiFi LoRa 32 V3**:

```cpp
#define LORA_NSS    8   // SPI Chip Select
#define LORA_NRESET 12  // Reset pin
#define LORA_BUSY   13  // Busy pin
#define LORA_DIO1   14  // Digital I/O 1
#define LORA_SCK    9   // SPI Clock
#define LORA_MISO   11  // SPI MISO
#define LORA_MOSI   10  // SPI MOSI
#define LED_PIN     35  // Built-in LED
```

For **other ESP32 LoRa32 boards**, you may need to adjust these pins in the firmware source code.

## Flashing Process

### Step 1: Prepare the Hardware

1. **Connect the ESP32** to your computer via USB cable
2. **Verify connection** - The device should appear as `/dev/ttyUSB0`, `/dev/ttyACM0`, or similar
3. **Check permissions** (Linux):
   ```bash
   # Add yourself to the dialout group
   sudo usermod -a -G dialout $USER

   # Log out and back in, or run:
   newgrp dialout
   ```

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

1. **Open Serial Monitor**:
   - `Tools` > `Serial Monitor`
   - Set baud rate to `115200`

2. **Reset the device**:
   - Press the `RESET` button on the ESP32

3. **Check for startup message**:
   ```
   AT READY
   ```

4. **Test basic communication**:
   - Type `AT` and press Enter
   - You should receive: `OK`

## Post-Flash Configuration

### Display Verification

If your device has an OLED display, you should see:
- **Banner**: "GeekInsaneMX" at the top
- **Status**: Device state information
- **TX Power**: Current power setting
- **Frequency**: Current frequency setting

### Initial AT Command Test

Test the device with these basic commands:

```bash
# Open serial connection (replace /dev/ttyACM0 with your device)
screen /dev/ttyACM0 115200

# Test commands:
AT                    # Should respond: OK
AT+FREQ?             # Should respond: +FREQ: 931.9375
AT+POWER?            # Should respond: +POWER: 2
AT+STATUS?           # Should respond: +STATUS: READY
```

Press `Ctrl+A` then `K` to exit screen.

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
