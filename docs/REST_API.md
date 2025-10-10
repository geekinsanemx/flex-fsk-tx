# FLEX Paging Message Transmitter - REST API Documentation

Complete REST API reference for programmatic control of the FLEX paging message transmitter (v3 firmware).

> **Note**: REST API is available **only in v3 firmware** (both TTGO LoRa32 and Heltec WiFi LoRa 32 V2). For AT command interface compatible with all firmware versions, see [AT_COMMANDS.md](AT_COMMANDS.md). For web interface usage, see [USER_GUIDE.md](USER_GUIDE.md).

## 🔗 API Overview

The FLEX paging message transmitter provides a RESTful HTTP API for remote message transmission and device control. The API uses JSON payloads and HTTP Basic Authentication for secure programmatic access.

### Supported Hardware
- **TTGO LoRa32**: v3 firmware with full WiFi capabilities (ESP32 + SX1276)
- **Heltec WiFi LoRa 32 V2**: v3 firmware with full WiFi capabilities (ESP32 + SX1276)

Both devices support identical API functionality and message lengths up to 248 characters.

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

### Message Queue System
- **Queue Capacity**: Up to 25 concurrent message requests
- **Processing**: Automatic sequential transmission when device becomes idle
- **Queue Status**: Real-time feedback via HTTP response codes
- **Timeout**: 30 seconds per transmission

### v3.1+ Features

#### EMR (Emergency Message Resynchronization)
- **Automatic Sync**: Sends synchronization bursts before FLEX messages for improved pager reception
- **Trigger Conditions**: First message or after 10-minute timeout since last EMR
- **Sync Pattern**: {0xA5, 0x5A, 0xA5, 0x5A} transmitted at current radio settings
- **Transparent**: No API changes required - EMR handled automatically by firmware

#### Message Truncation
- **Auto-Truncation**: Messages longer than 248 characters are automatically truncated
- **Truncation Format**: Truncates to 245 characters and adds "..." (248 total)
- **Response Indication**: API responses include `"truncated": true` when truncation occurs
- **No Errors**: Long messages no longer return validation errors - they are processed gracefully

### v3.6+ Features

#### Grafana Integration (Webhook Endpoint)
- **Webhook URL**: `POST /api/v1/alerts` - Grafana Alertmanager compatible endpoint
- **Multi-Alert Support**: Single webhook call can process multiple alerts
- **Queue Integration**: Grafana alerts use the same 25-message queue as other API calls
- **Configuration**: Enable/disable via web interface at `/grafana` or `settings.grafana_enabled`
- **Authentication**: Uses same HTTP Basic Auth as main API
- **Payload Format**: Standard Grafana Alertmanager JSON format with automatic field extraction

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
| `message` | string | ✅ | 1-248 characters (auto-truncated if longer) | Message text (ASCII printable chars) |
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

**Immediate Transmission Response** (HTTP 200):
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

**Queued Message Response** (HTTP 202):
```json
{
  "status": "success",
  "message": "Message queued for transmission (position 3)",
  "capcode": 1234567,
  "text": "Your message text",
  "truncated": false,
  "frequency": 929.6625,
  "power": 10,
  "maildrop": false,
  "queue_position": 3
}
```

**Queued Message with Truncation** (HTTP 202):
```json
{
  "status": "success",
  "message": "Message truncated to 248 chars and queued for transmission",
  "capcode": 1234567,
  "text": "Your very long message that was truncated...",
  "truncated": true,
  "frequency": 929.6625,
  "power": 10,
  "maildrop": false,
  "queue_position": 1
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
| 200 | Success | Message transmitted immediately |
| 202 | Accepted | Message queued for transmission |
| 400 | Bad Request | Invalid JSON payload or parameter values |
| 401 | Unauthorized | Missing or invalid authentication |
| 503 | Service Unavailable | Device busy and queue is full (25 messages) |
| 500 | Internal Error | Device error or transmission failure |

### Grafana Webhook Endpoint

**Endpoint**: `POST /api/v1/alerts`

Receives Grafana Alertmanager webhook notifications and converts them to FLEX paging messages.

#### Request Format (Grafana Alertmanager)

```json
{
  "alerts": [
    {
      "status": "firing",
      "labels": {
        "alertname": "HighCPUUsage",
        "severity": "critical",
        "capcode": "1234567"
      },
      "annotations": {
        "summary": "CPU usage exceeds 90%",
        "description": "Server web-01 CPU at 95%"
      }
    }
  ]
}
```

#### Grafana Alert Field Mapping

| Grafana Field | FLEX Mapping | Required | Description |
|--------------|--------------|----------|-------------|
| `labels.capcode` | Target capcode | ✅ | Must be valid 7-10 digit capcode |
| `annotations.summary` or `labels.alertname` | Message text | ✅ | Alert message content |
| `status` | Message prefix | ❌ | Prepends "FIRING:" or "RESOLVED:" to message |
| `labels.severity` | Message prefix | ❌ | Prepends severity level (e.g., "CRITICAL:") |

#### Grafana Response Format

**Successful Processing** (HTTP 200):
```json
{
  "status": "success",
  "message": "Processed 3 alerts: 3 successful, 0 failed",
  "processed": 3,
  "successful": 3,
  "failed": 0,
  "details": [
    {
      "alert": "HighCPUUsage",
      "capcode": 1234567,
      "queued": true,
      "queue_position": 1
    }
  ]
}
```

**Grafana Disabled** (HTTP 503):
```json
{
  "error": "Grafana webhook service is disabled"
}
```

**Queue Full** (HTTP 503):
```json
{
  "status": "partial",
  "message": "Processed 2 of 5 alerts (queue full)",
  "successful": 2,
  "failed": 3
}
```

#### Grafana Configuration Example

```yaml
# alertmanager.yml
receivers:
  - name: 'flex-pager'
    webhook_configs:
      - url: 'http://192.168.1.100:16180/api/v1/alerts'
        http_config:
          basic_auth:
            username: 'username'
            password: 'password'
        send_resolved: true

route:
  receiver: 'flex-pager'
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 1h
  routes:
    - match:
        severity: critical
      receiver: 'flex-pager'
```

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

**Test Grafana Webhook**:
```bash
curl -X POST http://192.168.1.100:16180/api/v1/alerts \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{
    "alerts": [
      {
        "status": "firing",
        "labels": {
          "alertname": "TestAlert",
          "severity": "warning",
          "capcode": "1234567"
        },
        "annotations": {
          "summary": "This is a test alert from curl"
        }
      }
    ]
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

**Advanced Python Client with Grafana Support**:
```python
import requests
import json
import time
from datetime import datetime

class FlexAPI:
    def __init__(self, device_ip, username="username", password="password", port=16180):
        self.base_url = f"http://{device_ip}:{port}/"
        self.grafana_url = f"http://{device_ip}:{port}/api/v1/alerts"
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
        # Note: Messages longer than 248 characters are auto-truncated by firmware

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

    def send_grafana_alert(self, alerts):
        """Send Grafana-formatted alerts to webhook endpoint"""

        payload = {"alerts": alerts}

        try:
            response = self.session.post(self.grafana_url, json=payload, timeout=30)
            return self._handle_response(response)
        except requests.exceptions.Timeout:
            raise Exception("Grafana webhook timeout")
        except requests.exceptions.ConnectionError:
            raise Exception("Connection failed - check device IP and network")

    def _handle_response(self, response):
        """Handle API response and extract data"""
        data = response.json()

        if response.status_code in [200, 202]:
            return {
                "success": True,
                "status_code": response.status_code,
                "data": data
            }
        else:
            raise Exception(f"API Error ({response.status_code}): {data.get('error', 'Unknown error')}")

# Usage - Standard Message
api = FlexAPI("192.168.1.100", "admin", "mypassword")

try:
    result = api.send_message(1234567, "Advanced Python client test", power=15, maildrop=True)
    print(f"Message sent successfully: {result['data']}")
except Exception as e:
    print(f"Error: {e}")

# Usage - Grafana Alert
try:
    alerts = [
        {
            "status": "firing",
            "labels": {
                "alertname": "HighMemory",
                "severity": "critical",
                "capcode": "1234567"
            },
            "annotations": {
                "summary": "Memory usage exceeds 90%"
            }
        }
    ]
    result = api.send_grafana_alert(alerts)
    print(f"Grafana alerts processed: {result['data']}")
except Exception as e:
    print(f"Grafana error: {e}")
```

### JavaScript/Node.js Integration

```javascript
const axios = require('axios');

class FlexAPI {
    constructor(deviceIP, username = 'username', password = 'password', port = 16180) {
        this.baseURL = `http://${deviceIP}:${port}/`;
        this.grafanaURL = `http://${deviceIP}:${port}/api/v1/alerts`;
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

    async sendGrafanaAlert(alerts) {
        const payload = { alerts: alerts };

        try {
            const response = await axios.post(this.grafanaURL, payload, {
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

// Usage - Standard Message
async function sendMessage() {
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

// Usage - Grafana Alert
async function sendGrafanaAlert() {
    const api = new FlexAPI('192.168.1.100', 'admin', 'password');

    const alerts = [
        {
            status: 'firing',
            labels: {
                alertname: 'DiskSpaceLow',
                severity: 'warning',
                capcode: '1234567'
            },
            annotations: {
                summary: 'Disk space below 10%'
            }
        }
    ];

    try {
        const result = await api.sendGrafanaAlert(alerts);

        if (result.success) {
            console.log('Grafana alerts processed:', result.data);
        } else {
            console.error('Error:', result.error);
        }
    } catch (e) {
        console.error('Exception:', e.message);
    }
}

sendMessage();
sendGrafanaAlert();
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

# Function to send Grafana alert
send_grafana_alert() {
    local capcode=$1
    local alert_name=$2
    local summary=$3
    local severity=${4:-warning}

    local payload=$(cat <<EOF
{
    "alerts": [
        {
            "status": "firing",
            "labels": {
                "alertname": "$alert_name",
                "severity": "$severity",
                "capcode": "$capcode"
            },
            "annotations": {
                "summary": "$summary"
            }
        }
    ]
}
EOF
)

    curl -s -X POST "http://$DEVICE_IP:$API_PORT/api/v1/alerts" \
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

echo -e "\nSending Grafana alert..."
send_grafana_alert 1234567 "HighCPU" "CPU usage exceeds 90%" "critical"
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

### Grafana Integration Configuration

**Enable/Disable Grafana Webhook**:
```bash
# Enable Grafana webhook via AT commands
AT+GRAFANA=1
AT+SAVE

# Disable Grafana webhook
AT+GRAFANA=0
AT+SAVE

# Check Grafana status
AT+DEVICE
# Response includes: +DEVICE_GRAFANA: Enabled
```

**Web Interface Configuration**:
- Navigate to `http://<device-ip>/grafana`
- Toggle Grafana integration on/off
- View webhook URL and configuration examples
- Test webhook endpoint with sample alerts

### Device Discovery

**Find Device IP Address**:
```bash
# Method 1: Check AT command response
AT+WIFI?

# Method 2: Network scan
nmap -sn 192.168.1.0/24 | grep -B2 "TTGO\|ESP32\|Heltec"

# Method 3: Router admin interface
# Check DHCP client list for "ESP32" or device MAC

# Method 4: Check device OLED display
# IP address shown on display when connected to WiFi
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

**Queue Full (503)**:
```json
{
  "status": "error",
  "message": "Device is busy and queue is full. Please try again later.",
  "max_queue_size": 25
}
```

**Grafana Webhook Disabled (503)**:
```json
{
  "error": "Grafana webhook service is disabled"
}
```

### Quick Troubleshooting

1. **Connection Issues**: Test connectivity with `ping` and verify port accessibility
2. **Authentication Issues**: Verify credentials via AT commands (`AT+APIUSER?`, `AT+APIPASS?`)
3. **Parameter Validation**: Check capcode (1-4,294,967,295), frequency (400-1000 MHz), power (0-20 dBm). Message length is auto-truncated at 248 characters
4. **Grafana Issues**: Verify Grafana is enabled via web interface or `AT+DEVICE` command
5. **Queue Issues**: Maximum 25 messages can be queued - wait for transmission to complete or reduce alert frequency

### Rate Limiting & Best Practices

1. **Sequential Transmission**: Wait for previous transmission to complete or use queue system (up to 25 messages)
2. **Error Handling**: Implement retry logic with exponential backoff
3. **Timeout Management**: Set appropriate request timeouts (30s recommended)
4. **Connection Pooling**: Reuse HTTP connections for multiple requests
5. **Status Monitoring**: Check device status before transmission
6. **Grafana Alerts**: Configure alert grouping to avoid overwhelming the 25-message queue
7. **Message Truncation**: Keep messages under 248 characters for full content delivery

## 📊 Hardware Specifications

### TTGO LoRa32
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio**: SX1276 (433/868/915 MHz)
- **Power**: 0 to +20 dBm
- **Serial Port**: `/dev/ttyACM0` (Linux), `COM3+` (Windows)
- **Default Frequency**: 915.0 MHz
- **Display**: 128x64 OLED (I2C)
- **WiFi**: 802.11 b/g/n (2.4GHz)
- **Message Limit**: 248 characters

### Heltec WiFi LoRa 32 V2
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio**: SX1276 (433/868/915 MHz)
- **Power**: 0 to +20 dBm
- **Serial Port**: `/dev/ttyUSB0` (Linux), `COM4+` (Windows)
- **Default Frequency**: 915.0 MHz
- **Display**: 128x64 OLED (I2C)
- **WiFi**: 802.11 b/g/n (2.4GHz)
- **Message Limit**: 248 characters
- **Battery**: Built-in LiPo charging circuit

**Note**: Both devices use the SX1276 radio chipset and support identical functionality. The Heltec V2 includes battery management features not present in the TTGO device.

## 📚 Related Documentation

- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command reference
- **[USER_GUIDE.md](USER_GUIDE.md)**: Web interface user guide
- **[README.md](../README.md)**: Project overview and setup
- **[FIRMWARE.md](FIRMWARE.md)**: Firmware installation guide

## 🔧 API Versioning & Compatibility

- **API Version**: v1 (current)
- **Firmware Requirement**: v3 only
- **Current Firmware Version**: v3.6.68
- **New Features (v3.6+)**: Grafana webhook endpoint, 25-message queue, enhanced logging
- **Breaking Changes**: None
- **Deprecated Features**: None

For backward compatibility with v1/v2 firmware, use AT command interface documented in [AT_COMMANDS.md](AT_COMMANDS.md).

## 🆕 v3.6 New Features Summary

### Grafana Webhook Integration
- **Endpoint**: `POST /api/v1/alerts`
- **Purpose**: Receive Grafana Alertmanager notifications and convert to FLEX pages
- **Configuration**: Web interface at `/grafana` with enable/disable toggle
- **Alert Mapping**: Automatic extraction of capcode from labels, message from annotations
- **Multi-Alert**: Process multiple alerts in single webhook call
- **Queue Integration**: Uses same 25-message queue as standard API

### Enhanced Message Queue
- **Capacity**: Increased from 10 to 25 messages
- **Benefit**: Better handling of burst traffic from monitoring systems
- **Status**: Queue position included in all 202 responses
- **Full Queue Handling**: Returns 503 when all 25 slots are full

### Improved Device Discovery
- **OLED Display**: Shows IP address and connection status
- **AT Commands**: Enhanced status reporting with `AT+DEVICE`
- **Web Interface**: Configuration pages show current network settings
