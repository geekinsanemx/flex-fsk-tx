# FLEX Paging Message Transmitter - REST API Documentation

Complete REST API reference for programmatic control of the FLEX paging message transmitter (v3 firmware).

> **Note**: REST API is available **only in v3 firmware** (TTGO LoRa32-OLED). For AT command interface compatible with all firmware versions, see [AT_COMMANDS.md](AT_COMMANDS.md). For web interface usage, see [USER_GUIDE.md](USER_GUIDE.md).

## 🔗 API Overview

The FLEX paging message transmitter provides a RESTful HTTP API for remote message transmission and device control. The API uses JSON payloads and HTTP Basic Authentication for secure programmatic access.

### Supported Hardware
- **TTGO LoRa32-OLED**: v3 firmware only (standalone with WiFi capabilities)

### Base URL
```
http://<device-ip>:<api-port>/
```

**Default Configuration**:
- **Port**: 16180 (configurable via `AT+APIPORT`)
- **Protocol**: HTTP (no HTTPS support)
- **Content-Type**: `application/json`

### Authentication
- **Method**: HTTP Basic Authentication
- **Default Credentials**: `username:password`
- **Header Format**: `Authorization: Basic <base64-encoded-credentials>`
- **Configuration**: Modify via AT commands (`AT+APIUSER`, `AT+APIPASS`)

### Rate Limiting
- **Concurrent Requests**: 1 (device processes one transmission at a time)
- **Transmission Queue**: Not supported (requests during transmission return error)
- **Timeout**: 30 seconds per transmission

## 📡 API Endpoints

### Send FLEX Message

**Endpoint**: `POST /`

Transmits a FLEX paging message with specified parameters.

#### Request Format

```json
{
  "capcode": 1234567,
  "frequency": 929.6625,
  "power": 10,
  "message": "Your message text",
  "maildrop": false
}
```

#### JSON Payload Attributes

| Attribute | Type | Required | Range/Format | Description |
|-----------|------|----------|--------------|-------------|
| `capcode` | integer | ✅ | 1 - 4,294,967,295 | Target FLEX capcode (7-10 digits) |
| `frequency` | number | ✅ | 400.0 - 1000.0 | Transmission frequency in MHz |
| `power` | integer | ✅ | 0 - 20 | Transmit power in dBm |
| `message` | string | ✅ | 1-240 characters | Message text (ASCII printable chars) |
| `maildrop` | boolean | ❌ | true/false | Mail drop flag (default: false) |

#### Frequency Format Support

The API accepts frequency in two formats with automatic conversion:

```json
// MHz format (recommended)
{"frequency": 929.6625}

// Hz format (auto-converted)
{"frequency": 929662500}
```

**Conversion Rule**: Values > 1000 are treated as Hz and divided by 1,000,000.

#### Response Format

**Success Response** (HTTP 200):
```json
{
  "status": "success",
  "message": "Message transmitted successfully",
  "capcode": 1234567,
  "frequency": 929.6625,
  "power": 10,
  "maildrop": false,
  "transmission_time": "2024-01-15T10:30:45Z"
}
```

**Error Response** (HTTP 400/401/500):
```json
{
  "status": "error",
  "error": "Invalid capcode range",
  "details": "Capcode must be between 1 and 4294967295"
}
```

#### HTTP Status Codes

| Code | Status | Description |
|------|--------|-------------|
| 200 | Success | Message transmitted successfully |
| 400 | Bad Request | Invalid JSON payload or parameter values |
| 401 | Unauthorized | Missing or invalid authentication |
| 409 | Conflict | Device busy (transmission in progress) |
| 500 | Internal Error | Device error or transmission failure |

## 🔧 Programming Examples

### cURL Examples

**Basic Message Transmission**:
```bash
curl -X POST http://192.168.1.100:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929.6625,
    "power": 10,
    "message": "Hello from REST API"
  }'
```

**Message with Mail Drop Flag**:
```bash
curl -X POST http://192.168.1.100:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929.6625,
    "power": 15,
    "message": "Urgent notification",
    "maildrop": true
  }'
```

**Using Hz Frequency Format**:
```bash
curl -X POST http://192.168.1.100:16180/ \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929662500,
    "power": 10,
    "message": "Hz format test"
  }'
```

### Python Integration

**Simple Message Sending**:
```python
import requests
import json

def send_flex_message(device_ip, capcode, message, frequency=929.6625, power=10, maildrop=False):
    url = f"http://{device_ip}:16180/"
    
    payload = {
        "capcode": capcode,
        "frequency": frequency,
        "power": power,
        "message": message,
        "maildrop": maildrop
    }
    
    try:
        response = requests.post(
            url,
            json=payload,
            auth=('username', 'password'),
            timeout=30
        )
        
        if response.status_code == 200:
            return {"success": True, "data": response.json()}
        else:
            return {"success": False, "error": response.json()}
            
    except requests.exceptions.RequestException as e:
        return {"success": False, "error": str(e)}

# Usage
result = send_flex_message("192.168.1.100", 1234567, "Python API test")
print(f"Success: {result['success']}")
```

**Advanced Python Client**:
```python
import requests
import json
import time
from datetime import datetime

class FlexAPI:
    def __init__(self, device_ip, username="username", password="password", port=16180):
        self.base_url = f"http://{device_ip}:{port}/"
        self.auth = (username, password)
        self.session = requests.Session()
        self.session.auth = self.auth
        
    def send_message(self, capcode, message, frequency=929.6625, power=10, maildrop=False):
        """Send FLEX message with validation and error handling"""
        
        # Validate parameters
        if not (1 <= capcode <= 4294967295):
            raise ValueError("Capcode must be between 1 and 4,294,967,295")
        if not (400.0 <= frequency <= 1000.0):
            raise ValueError("Frequency must be between 400.0 and 1000.0 MHz")
        if not (0 <= power <= 20):
            raise ValueError("Power must be between 0 and 20 dBm")
        if len(message) > 240:
            raise ValueError("Message must be 240 characters or less")
            
        payload = {
            "capcode": capcode,
            "frequency": frequency,
            "power": power,
            "message": message,
            "maildrop": maildrop
        }
        
        try:
            response = self.session.post(self.base_url, json=payload, timeout=30)
            return self._handle_response(response)
        except requests.exceptions.Timeout:
            raise Exception("Request timeout - device may be busy")
        except requests.exceptions.ConnectionError:
            raise Exception("Connection failed - check device IP and network")
    
    def _handle_response(self, response):
        """Handle API response and extract data"""
        data = response.json()
        
        if response.status_code == 200:
            return {
                "success": True,
                "capcode": data.get("capcode"),
                "frequency": data.get("frequency"),
                "power": data.get("power"),
                "transmission_time": data.get("transmission_time")
            }
        else:
            raise Exception(f"API Error ({response.status_code}): {data.get('error', 'Unknown error')}")

# Usage
api = FlexAPI("192.168.1.100", "admin", "mypassword")

try:
    result = api.send_message(1234567, "Advanced Python client test", power=15, maildrop=True)
    print(f"Message sent successfully at {result['transmission_time']}")
except Exception as e:
    print(f"Error: {e}")
```

### JavaScript/Node.js Integration

```javascript
const axios = require('axios');

class FlexAPI {
    constructor(deviceIP, username = 'username', password = 'password', port = 16180) {
        this.baseURL = `http://${deviceIP}:${port}/`;
        this.auth = {
            username: username,
            password: password
        };
    }

    async sendMessage(capcode, message, options = {}) {
        const payload = {
            capcode: capcode,
            frequency: options.frequency || 929.6625,
            power: options.power || 10,
            message: message,
            maildrop: options.maildrop || false
        };

        try {
            const response = await axios.post(this.baseURL, payload, {
                auth: this.auth,
                headers: {
                    'Content-Type': 'application/json'
                },
                timeout: 30000
            });

            return {
                success: true,
                data: response.data
            };
        } catch (error) {
            if (error.response) {
                return {
                    success: false,
                    error: error.response.data
                };
            } else {
                return {
                    success: false,
                    error: error.message
                };
            }
        }
    }
}

// Usage
async function example() {
    const api = new FlexAPI('192.168.1.100', 'admin', 'password');
    
    try {
        const result = await api.sendMessage(1234567, 'Node.js API test', {
            frequency: 929.6625,
            power: 15,
            maildrop: true
        });
        
        if (result.success) {
            console.log('Message sent:', result.data);
        } else {
            console.error('Error:', result.error);
        }
    } catch (e) {
        console.error('Exception:', e.message);
    }
}

example();
```

### Shell Script Integration

```bash
#!/bin/bash

# FLEX API Configuration
DEVICE_IP="192.168.1.100"
API_PORT="16180"
USERNAME="username"
PASSWORD="password"

# Function to send FLEX message
send_flex_message() {
    local capcode=$1
    local message=$2
    local frequency=${3:-929.6625}
    local power=${4:-10}
    local maildrop=${5:-false}
    
    local payload=$(cat <<EOF
{
    "capcode": $capcode,
    "frequency": $frequency,
    "power": $power,
    "message": "$message",
    "maildrop": $maildrop
}
EOF
)
    
    curl -s -X POST "http://$DEVICE_IP:$API_PORT/" \
        -u "$USERNAME:$PASSWORD" \
        -H "Content-Type: application/json" \
        -d "$payload" \
        --max-time 30
}

# Usage examples
echo "Sending standard message..."
send_flex_message 1234567 "Shell script test"

echo -e "\nSending urgent message with mail drop..."
send_flex_message 1234567 "Urgent alert" 929.6625 15 true

echo -e "\nSending message with custom frequency..."
send_flex_message 8901234 "Custom frequency test" 931.9375 10 false
```

## ⚙️ API Configuration

### Authentication Management

**Set Custom Credentials via AT Commands**:
```bash
# Set username and password
AT+APIUSER=admin
AT+APIPASS=secure_password_123

# Set custom API port
AT+APIPORT=8080

# Save configuration to EEPROM
AT+SAVE
```

### Network Configuration

**Configure WiFi via AT Commands**:
```bash
# Connect to WiFi network
AT+WIFI=YourNetwork,YourPassword

# Check connection status
AT+WIFI?
# Response: +WIFI: CONNECTED,192.168.1.100

# Check API configuration
AT+APIPORT?
AT+APIUSER?
```

### Device Discovery

**Find Device IP Address**:
```bash
# Method 1: Check AT command response
AT+WIFI?

# Method 2: Network scan
nmap -sn 192.168.1.0/24 | grep -B2 "TTGO\|ESP32"

# Method 3: Router admin interface
# Check DHCP client list for "ESP32" or device MAC
```

## 🚨 Error Handling & Troubleshooting

**🔧 Complete Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for comprehensive REST API issue resolution covering network connectivity, authentication problems, device errors, and professional problem reporting.

### Quick API Error Responses

**Invalid Authentication (401)**:
```json
{
  "status": "error",
  "error": "Authentication failed",
  "details": "Invalid username or password"
}
```

**Parameter Validation Error (400)**:
```json
{
  "status": "error",
  "error": "Invalid frequency",
  "details": "Frequency must be between 400.0 and 1000.0 MHz"
}
```

**Device Busy (409)**:
```json
{
  "status": "error",
  "error": "Device busy",
  "details": "Another transmission is in progress"
}
```

### Quick Troubleshooting

1. **Connection Issues**: Test connectivity with `ping` and verify port accessibility
2. **Authentication Issues**: Verify credentials via AT commands (`AT+APIUSER?`, `AT+APIPASS?`)
3. **Parameter Validation**: Check capcode (1-4,294,967,295), frequency (400-1000 MHz), power (0-20 dBm), message length (≤240 chars)

### Rate Limiting & Best Practices

1. **Sequential Transmission**: Wait for previous transmission to complete
2. **Error Handling**: Implement retry logic with exponential backoff
3. **Timeout Management**: Set appropriate request timeouts (30s recommended)
4. **Connection Pooling**: Reuse HTTP connections for multiple requests
5. **Status Monitoring**: Check device status before transmission

## 📚 Related Documentation

- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface user guide
- **[README.md](README.md)**: Project overview and setup
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware installation guide

## 🔧 API Versioning & Compatibility

- **API Version**: v1 (current)
- **Firmware Requirement**: v3 only
- **Breaking Changes**: None planned
- **Deprecated Features**: None

For backward compatibility with v1/v2 firmware, use AT command interface documented in [AT_COMMANDS.md](AT_COMMANDS.md).