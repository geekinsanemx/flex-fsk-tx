# TTGO FLEX Transmitter - AT Commands Guide

Complete guide for using the AT command interface to control the TTGO FLEX paging transmitter via serial communication.

> **Note**: AT commands are available in all firmware versions (v1, v2, v3). This guide includes WiFi and advanced commands specific to v3 firmware.

## 🔗 Connection Setup

### Hardware Connection
- **Interface**: USB Serial (Virtual COM Port)
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

### Software Options

#### Arduino IDE Serial Monitor
1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Set line ending to **Both NL & CR**
4. Type commands in the input field

#### PuTTY (Windows)
```
Connection Type: Serial
Serial Line: COM3 (adjust for your port)
Speed: 115200
Data bits: 8
Stop bits: 1
Parity: None
Flow control: None
```

#### Screen (Linux/macOS)
```bash
screen /dev/ttyUSB0 115200
# or
screen /dev/ttyACM0 115200
```

#### Minicom (Linux)
```bash
minicom -b 115200 -D /dev/ttyUSB0
```

#### Python Terminal
```python
import serial
import time

# Open serial connection
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
time.sleep(2)  # Wait for connection

# Send command
ser.write(b'AT+VERSION\r\n')
response = ser.readline().decode('utf-8').strip()
print(response)

ser.close()
```

## 📝 Command Format

### Basic Syntax
- **Command Format**: `AT+COMMAND[=parameter]`
- **Line Ending**: Carriage Return + Line Feed (CRLF)
- **Case Sensitivity**: Commands are case-insensitive
- **Response Format**: Data lines followed by `OK` or `ERROR`

### Response Types
- **`OK`**: Command executed successfully
- **`ERROR`**: Command failed or invalid parameter
- **`SENDING`**: Message transmission started
- **`BUSY`**: Device currently transmitting
- **`+DATA:`**: Information response with data

### Example Interaction
```
→ AT+VERSION
← +VERSION: TTGO FLEX Transmitter v3.0
← OK

→ AT+FREQ=929.6625
← OK

→ AT+FREQ?
← +FREQ: 929.6625
← OK
```

## 📋 Complete Command Reference

### System Information Commands

#### AT+VERSION
**Purpose**: Get firmware version information
**Syntax**: `AT+VERSION`
**Response**: `+VERSION: TTGO FLEX Transmitter v3.0`

```
AT+VERSION
+VERSION: TTGO FLEX Transmitter v3.0
OK
```

#### AT+STATUS
**Purpose**: Display comprehensive device status
**Syntax**: `AT+STATUS`
**Response**: Multi-line system information

```
AT+STATUS
+STATUS: Device Information
Banner: flex-fsk-tx
Frequency: 929.6625 MHz
TX Power: 10 dBm
Default Capcode: 1234567
Uptime: 1440 minutes
Free Heap: 182456 bytes
Theme: Default
WiFi: Enabled
API Port: 16180
Chip: ESP32-D0WDQ6 rev 1.0
CPU Freq: 240 MHz
Battery: 4.15V (89%)
+STATUS: Network Information
WiFi SSID: MyNetwork
IP Address: 192.168.1.100
Subnet: 255.255.255.0
Gateway: 192.168.1.1
DNS: 8.8.8.8
MAC: AA:BB:CC:DD:EE:FF
RSSI: -45 dBm
DHCP: Enabled
OK
```

#### AT+BATTERY
**Purpose**: Get battery voltage and percentage
**Syntax**: `AT+BATTERY`
**Response**: `+BATTERY: voltage percentage`

```
AT+BATTERY
+BATTERY: 4.15V 89%
OK
```

### Frequency Control Commands

#### AT+FREQ
**Purpose**: Set or query transmission frequency
**Syntax**: 
- Set: `AT+FREQ=<frequency>`
- Query: `AT+FREQ?`
**Parameters**: 
- `frequency`: 400.0 to 1000.0 (MHz)
**Examples**:

```
AT+FREQ=929.6625
OK

AT+FREQ?
+FREQ: 929.6625
OK

AT+FREQ=2000.0
ERROR: Frequency must be between 400.0 and 1000.0 MHz
```

### Power Control Commands

#### AT+POWER
**Purpose**: Set or query transmission power
**Syntax**: 
- Set: `AT+POWER=<power>`
- Query: `AT+POWER?`
**Parameters**: 
- `power`: 0 to 20 (dBm)
**Examples**:

```
AT+POWER=15
OK

AT+POWER?
+POWER: 15
OK

AT+POWER=25
ERROR: TX Power must be between 0 and 20 dBm
```

### Capcode Commands

#### AT+CAPCODE
**Purpose**: Set or query default capcode
**Syntax**: 
- Set: `AT+CAPCODE=<capcode>`
- Query: `AT+CAPCODE?`
**Parameters**: 
- `capcode`: 1 to 4294967295
**Examples**:

```
AT+CAPCODE=1234567
OK

AT+CAPCODE?
+CAPCODE: 1234567
OK
```

### Message Transmission Commands

#### AT+SEND
**Purpose**: Send alphanumeric message with current settings
**Syntax**: `AT+SEND=<message>`
**Parameters**: 
- `message`: 1-240 characters, ASCII text
**Response**: `SENDING` during transmission

```
AT+SEND=Hello World
SENDING
OK

AT+SEND=This is a test message from TTGO FLEX transmitter
SENDING
OK
```

#### AT+SENDBIN
**Purpose**: Send binary message data
**Syntax**: `AT+SENDBIN=<length>`
**Parameters**: 
- `length`: Message length in bytes (1-240)
**Process**: 
1. Send command with length
2. Wait for `READY` response
3. Send exact number of bytes specified
4. Device responds with `SENDING`

```
AT+SENDBIN=12
READY
Hello Binary
SENDING
OK
```

#### AT+MAILDROP
**Purpose**: Enable or disable mail drop flag for next message
**Syntax**: 
- Set: `AT+MAILDROP=<flag>`
- Query: `AT+MAILDROP?`
**Parameters**: 
- `flag`: 0 (disabled) or 1 (enabled)

```
AT+MAILDROP=1
OK

AT+SEND=Urgent message with mail drop
SENDING
OK

AT+MAILDROP?
+MAILDROP: 0
OK
```

### WiFi Management Commands

#### AT+WIFIENABLE
**Purpose**: Enable or disable WiFi functionality
**Syntax**: 
- Set: `AT+WIFIENABLE=<enable>`
- Query: `AT+WIFIENABLE?`
**Parameters**: 
- `enable`: 0 (disable) or 1 (enable)

```
AT+WIFIENABLE=1
OK

AT+WIFIENABLE?
+WIFIENABLE: 1
OK
```

#### AT+WIFISSID
**Purpose**: Set WiFi network SSID
**Syntax**: `AT+WIFISSID=<ssid>`
**Parameters**: 
- `ssid`: Network name (up to 32 characters)

```
AT+WIFISSID=MyNetwork
OK
```

#### AT+WIFIPASS
**Purpose**: Set WiFi network password
**Syntax**: `AT+WIFIPASS=<password>`
**Parameters**: 
- `password`: Network password (up to 64 characters)

```
AT+WIFIPASS=MySecurePassword123
OK
```

#### AT+WIFICONNECT
**Purpose**: Attempt to connect to configured WiFi network
**Syntax**: `AT+WIFICONNECT`
**Response**: Connection status

```
AT+WIFICONNECT
+WIFI: Connecting to MyNetwork...
+WIFI: Connected successfully
+WIFI: IP Address: 192.168.1.100
OK
```

#### AT+WIFIDISCONNECT
**Purpose**: Disconnect from WiFi and enter AP mode
**Syntax**: `AT+WIFIDISCONNECT`

```
AT+WIFIDISCONNECT
+WIFI: Disconnected
+WIFI: Starting AP mode
+WIFI: AP SSID: TTGO_FLEX_A1B2
+WIFI: AP IP: 192.168.4.1
OK
```

#### AT+WIFIDHCP
**Purpose**: Enable or disable DHCP
**Syntax**: 
- Set: `AT+WIFIDHCP=<enable>`
- Query: `AT+WIFIDHCP?`
**Parameters**: 
- `enable`: 0 (static IP) or 1 (DHCP)

```
AT+WIFIDHCP=0
OK
```

#### AT+WIFIIP
**Purpose**: Set static IP address
**Syntax**: `AT+WIFIIP=<ip>`
**Parameters**: 
- `ip`: IP address in dotted decimal notation

```
AT+WIFIIP=192.168.1.100
OK
```

#### AT+WIFINETMASK
**Purpose**: Set subnet mask
**Syntax**: `AT+WIFINETMASK=<netmask>`
**Parameters**: 
- `netmask`: Subnet mask in dotted decimal notation

```
AT+WIFINETMASK=255.255.255.0
OK
```

#### AT+WIFIGATEWAY
**Purpose**: Set network gateway
**Syntax**: `AT+WIFIGATEWAY=<gateway>`
**Parameters**: 
- `gateway`: Gateway IP address

```
AT+WIFIGATEWAY=192.168.1.1
OK
```

#### AT+WIFIDNS
**Purpose**: Set DNS server
**Syntax**: `AT+WIFIDNS=<dns>`
**Parameters**: 
- `dns`: DNS server IP address

```
AT+WIFIDNS=8.8.8.8
OK
```

### Device Configuration Commands

#### AT+BANNER
**Purpose**: Set custom banner message for OLED display
**Syntax**: 
- Set: `AT+BANNER=<message>`
- Query: `AT+BANNER?`
**Parameters**: 
- `message`: 1-16 characters for display

```
AT+BANNER=KD8ABC
OK

AT+BANNER?
+BANNER: KD8ABC
OK
```

#### AT+THEME
**Purpose**: Set user interface theme
**Syntax**: 
- Set: `AT+THEME=<theme>`
- Query: `AT+THEME?`
**Parameters**: 
- `theme`: 0 (Default/Blue), 1 (Light), 2 (Dark)

```
AT+THEME=2
OK

AT+THEME?
+THEME: 2
OK
```

#### AT+APIPORT
**Purpose**: Set REST API listening port
**Syntax**: 
- Set: `AT+APIPORT=<port>`
- Query: `AT+APIPORT?`
**Parameters**: 
- `port`: 1024-65535

```
AT+APIPORT=8080
OK

AT+APIPORT?
+APIPORT: 8080
OK
```

#### AT+APIUSER
**Purpose**: Set API authentication username
**Syntax**: `AT+APIUSER=<username>`
**Parameters**: 
- `username`: Up to 32 characters

```
AT+APIUSER=admin
OK
```

#### AT+APIPASS
**Purpose**: Set API authentication password
**Syntax**: `AT+APIPASS=<password>`
**Parameters**: 
- `password`: Up to 64 characters

```
AT+APIPASS=newpassword123
OK
```

### System Control Commands

#### AT+SAVE
**Purpose**: Save current configuration to EEPROM
**Syntax**: `AT+SAVE`
**Response**: Configuration saved confirmation

```
AT+SAVE
+CONFIG: Configuration saved to EEPROM
OK
```

#### AT+LOAD
**Purpose**: Load configuration from EEPROM
**Syntax**: `AT+LOAD`
**Response**: Configuration loaded confirmation

```
AT+LOAD
+CONFIG: Configuration loaded from EEPROM
OK
```

#### AT+FACTORYRESET
**Purpose**: Reset device to factory defaults
**Syntax**: `AT+FACTORYRESET`
**Response**: Reset confirmation and restart

```
AT+FACTORYRESET
+RESET: Factory reset initiated
+RESET: EEPROM cleared
+RESET: Restarting device...
```

#### AT+RESTART
**Purpose**: Restart the device
**Syntax**: `AT+RESTART`
**Response**: Restart confirmation

```
AT+RESTART
+SYSTEM: Restarting device...
```

## 🔄 Command Sequences

### Initial Device Setup
```bash
# Check device version
AT+VERSION

# Set basic transmission parameters
AT+FREQ=929.6625
AT+POWER=10
AT+CAPCODE=1234567

# Configure WiFi
AT+WIFISSID=MyNetwork
AT+WIFIPASS=MyPassword
AT+WIFICONNECT

# Save configuration
AT+SAVE
```

### Quick Message Transmission
```bash
# Set frequency and power
AT+FREQ=929.6625
AT+POWER=10

# Send simple message
AT+SEND=Hello World

# Send urgent message with mail drop
AT+MAILDROP=1
AT+SEND=Emergency Alert
```

### Static IP Configuration
```bash
# Disable DHCP
AT+WIFIDHCP=0

# Set static network configuration
AT+WIFIIP=192.168.1.100
AT+WIFINETMASK=255.255.255.0
AT+WIFIGATEWAY=192.168.1.1
AT+WIFIDNS=8.8.8.8

# Connect to network
AT+WIFICONNECT

# Save configuration
AT+SAVE
```

### Device Customization
```bash
# Set custom banner
AT+BANNER=My FLEX TX

# Set dark theme
AT+THEME=2

# Change API port
AT+APIPORT=8080

# Update API credentials
AT+APIUSER=operator
AT+APIPASS=securepass123

# Save all changes
AT+SAVE
```

## 🔧 Automation Scripts

### Bash Script Example
```bash
#!/bin/bash
# flex_at_control.sh

DEVICE="/dev/ttyUSB0"
BAUD="115200"

# Function to send AT command
send_at() {
    echo "$1" > $DEVICE
    sleep 1
    cat $DEVICE | head -10
}

# Setup device
echo "Configuring FLEX transmitter..."
send_at "AT+FREQ=929.6625"
send_at "AT+POWER=10"
send_at "AT+CAPCODE=1234567"

# Send message
send_at "AT+SEND=Automated message from bash script"

echo "Configuration complete!"
```

### Python Automation Script
```python
#!/usr/bin/env python3
import serial
import time
import sys

class FlexTransmitter:
    def __init__(self, port='/dev/ttyUSB0', baud=115200):
        self.ser = serial.Serial(port, baud, timeout=2)
        time.sleep(2)  # Wait for device initialization
        
    def send_command(self, command):
        """Send AT command and return response"""
        self.ser.write(f"{command}\r\n".encode())
        time.sleep(0.5)
        
        response = []
        while self.ser.in_waiting:
            line = self.ser.readline().decode('utf-8').strip()
            if line:
                response.append(line)
                
        return response
    
    def configure(self, freq=929.6625, power=10, capcode=1234567):
        """Configure basic transmission parameters"""
        commands = [
            f"AT+FREQ={freq}",
            f"AT+POWER={power}",
            f"AT+CAPCODE={capcode}"
        ]
        
        for cmd in commands:
            response = self.send_command(cmd)
            print(f"{cmd}: {response}")
            
    def send_message(self, message, mail_drop=False):
        """Send FLEX message"""
        if mail_drop:
            self.send_command("AT+MAILDROP=1")
            
        response = self.send_command(f"AT+SEND={message}")
        print(f"Message sent: {response}")
        
    def get_status(self):
        """Get device status"""
        return self.send_command("AT+STATUS")
        
    def close(self):
        """Close serial connection"""
        self.ser.close()

# Usage example
if __name__ == "__main__":
    flex = FlexTransmitter("/dev/ttyUSB0")
    
    # Configure device
    flex.configure(freq=929.6625, power=10, capcode=1234567)
    
    # Send test message
    flex.send_message("Test from Python script")
    
    # Get status
    status = flex.get_status()
    for line in status:
        print(line)
        
    flex.close()
```

### Windows PowerShell Script
```powershell
# flex_control.ps1
param(
    [string]$Port = "COM3",
    [int]$Baud = 115200,
    [string]$Message = "Test message"
)

# Open serial port
$port = New-Object System.IO.Ports.SerialPort $Port, $Baud
$port.Open()
Start-Sleep -Seconds 2

function Send-ATCommand {
    param([string]$Command)
    
    $port.WriteLine($Command)
    Start-Sleep -Milliseconds 500
    
    $response = ""
    while ($port.BytesToRead -gt 0) {
        $response += $port.ReadLine() + "`n"
    }
    
    Write-Host "$Command : $response"
    return $response
}

# Configure device
Send-ATCommand "AT+FREQ=929.6625"
Send-ATCommand "AT+POWER=10"
Send-ATCommand "AT+CAPCODE=1234567"

# Send message
Send-ATCommand "AT+SEND=$Message"

# Close port
$port.Close()
Write-Host "Message transmission complete"
```

## 🚨 Error Handling

### Common Error Messages

**Frequency Errors**:
```
AT+FREQ=2000.0
ERROR: Frequency must be between 400.0 and 1000.0 MHz
```

**Power Errors**:
```
AT+POWER=25
ERROR: TX Power must be between 0 and 20 dBm
```

**Device Busy**:
```
AT+SEND=Test
BUSY: Device currently transmitting
```

**WiFi Errors**:
```
AT+WIFICONNECT
ERROR: WiFi SSID not configured
```

**Invalid Command**:
```
AT+INVALID
ERROR: Unknown command
```

### Error Recovery Strategies

**For BUSY errors**:
```python
def send_with_retry(flex, message, max_retries=3):
    for attempt in range(max_retries):
        response = flex.send_command(f"AT+SEND={message}")
        if "BUSY" not in str(response):
            return response
        time.sleep(1)  # Wait before retry
    return "ERROR: Max retries exceeded"
```

**For connection errors**:
```python
def safe_connect():
    try:
        flex = FlexTransmitter("/dev/ttyUSB0")
        return flex
    except serial.SerialException:
        print("Device not found, trying alternative ports...")
        for port in ["/dev/ttyACM0", "/dev/ttyUSB1"]:
            try:
                return FlexTransmitter(port)
            except:
                continue
    return None
```

## 🔒 Security Considerations

### Serial Port Access
- Ensure proper permissions for serial port access
- Use secure terminal applications
- Avoid logging sensitive configuration data

### Password Handling
```bash
# Secure way to set passwords
read -s -p "Enter API password: " PASSWORD
echo "AT+APIPASS=$PASSWORD" > /dev/ttyUSB0
unset PASSWORD
```

### Configuration Backup
```python
def backup_config(flex, filename):
    """Backup device configuration"""
    config = {
        'banner': flex.send_command("AT+BANNER?"),
        'freq': flex.send_command("AT+FREQ?"),
        'power': flex.send_command("AT+POWER?"),
        'capcode': flex.send_command("AT+CAPCODE?"),
        'theme': flex.send_command("AT+THEME?")
    }
    
    with open(filename, 'w') as f:
        json.dump(config, f, indent=2)
```

## 🧪 Testing Commands

### Validation Test Suite
```bash
#!/bin/bash
# at_test_suite.sh

echo "=== FLEX Transmitter AT Command Test Suite ==="

# Test basic commands
echo "Testing basic commands..."
echo "AT+VERSION" > /dev/ttyUSB0
sleep 1

# Test parameter validation
echo "Testing frequency validation..."
echo "AT+FREQ=2000.0" > /dev/ttyUSB0  # Should fail
sleep 1

echo "AT+FREQ=929.6625" > /dev/ttyUSB0  # Should succeed
sleep 1

# Test power validation
echo "Testing power validation..."
echo "AT+POWER=25" > /dev/ttyUSB0  # Should fail
sleep 1

echo "AT+POWER=10" > /dev/ttyUSB0  # Should succeed
sleep 1

echo "=== Test complete ==="
```

---

**Related Documentation**: 
- [USER.md](USER.md) - Complete user interface guide
- [API.md](API.md) - REST API documentation
- [FIRMWARE.md](FIRMWARE.md) - Build and deployment guide
- [README.md](README.md) - Project overview