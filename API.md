# FLEX Transmitter - REST API Documentation

Complete REST API reference for programmatic control of the FLEX paging transmitter.

> **Note**: REST API is available **only in v3 firmware** (TTGO LoRa32-OLED). For AT command interface compatible with all firmware versions, see [AT_COMMANDS.md](AT_COMMANDS.md).

## 🔗 API Overview

The FLEX transmitter provides a RESTful HTTP API for remote message transmission. The API uses JSON payloads and HTTP Basic Authentication for secure access.

### Supported Hardware
- **TTGO LoRa32-OLED**: v3 firmware only (standalone with WiFi capabilities)

> **Note**: Heltec devices with v2 firmware support remote encoding via AT commands but do not have REST API capabilities.

### Base URL
```
http://device-ip:16180/
```

### Authentication
- **Method**: HTTP Basic Authentication
- **Default Username**: `admin`
- **Default Password**: `password`
- **Header**: `Authorization: Basic <base64-encoded-credentials>`
- **Configuration**: Use AT commands or web interface to set custom credentials

### Content Type
- **Request**: `application/json`
- **Response**: `application/json`

### v3 Firmware Features
- **Encoding**: Both local and remote encoding support
- **REST API**: Full HTTP JSON API with authentication
- **WiFi Integration**: Built-in WiFi with AP mode for configuration
- **Web Interface**: Complete web-based control interface with mail drop support
- **EEPROM Configuration**: Persistent settings storage
- **Mail Drop**: Available in all interfaces (AT commands, web, API)

## 📡 API Endpoints

### Send Message

**Endpoint**: `POST /`

Transmits a FLEX paging message with specified parameters.

#### Request Format

```json
{
  "capcode": 1234567,
  "frequency": 929.6625,
  "power": 10,
  "message": "Hello from API",
  "mail_drop": false
}
```

#### Parameters

| Parameter | Type | Required | Range/Format | Description |
|-----------|------|----------|--------------|-------------|
| `capcode` | integer | ✅ | 1 - 4,294,967,295 | Target pager capcode |
| `frequency` | number | ✅ | 400.0 - 1000.0 MHz<br/>or 400000000 - 1000000000 Hz | Transmission frequency |
| `power` | integer | ❌ | 0 - 20 dBm (TTGO) | Transmission power (uses device default if not provided) |
| `message` | string | ✅ | 1 - 240 characters | Message content (ASCII) |
| `mail_drop` | boolean | ❌ | true/false | Mail drop flag (default: false) |

#### Frequency Formats

The API accepts frequency in two formats:

**MHz Format** (recommended):
```json
"frequency": 929.6625
```

**Hz Format** (auto-converted):
```json
"frequency": 929662500
```

#### Response Format

**Success (200 OK)**:
```json
{
  "status": "success",
  "message": "Transmission started",
  "frequency": 929.6625,
  "power": 10,
  "capcode": 1234567,
  "text": "Hello from API"
}
```

**Error (400 Bad Request)**:
```json
{
  "error": "Missing required fields: capcode, frequency, message"
}
```

**Error (500 Internal Server Error)**:
```json
{
  "status": "error",
  "message": "Failed to start transmission: -2",
  "frequency": 929.6625,
  "power": 10,
  "capcode": 1234567,
  "text": "Hello from API"
}
```

**Error (401 Unauthorized)**:
```json
{
  "error": "Authentication required"
}
```

**Error (503 Service Unavailable)**:
```json
{
  "error": "Device is busy"
}
```

## 💻 Usage Examples

### cURL Examples

#### Basic Message
```bash
curl -X POST http://192.168.1.100:16180/ \
  -u admin:password \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929.6625,
    "power": 10,
    "message": "Hello World"
  }'
```

#### Message with Mail Drop
```bash
curl -X POST http://192.168.1.100:16180/ \
  -u admin:password \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929.6625,
    "power": 15,
    "message": "Urgent: Server down!",
    "mail_drop": true
  }'
```

#### Frequency in Hz Format
```bash
curl -X POST http://192.168.1.100:16180/ \
  -u admin:password \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929662500,
    "power": 10,
    "message": "Frequency in Hz"
  }'
```

### Python Examples

#### Simple Python Script
```python
import requests
import json

# Device configuration
device_ip = "192.168.1.100"
api_port = 16180
username = "admin"
password = "password"

# API endpoint
url = f"http://{device_ip}:{api_port}/"

# Message data
data = {
    "capcode": 1234567,
    "frequency": 929.6625,
    "power": 10,
    "message": "Hello from Python!"
}

# Send request
try:
    response = requests.post(
        url, 
        auth=(username, password),
        headers={"Content-Type": "application/json"},
        json=data,
        timeout=10
    )
    
    if response.status_code == 200:
        print("Message sent successfully!")
        print(response.json())
    else:
        print(f"Error: {response.status_code}")
        print(response.json())
        
except requests.exceptions.RequestException as e:
    print(f"Connection error: {e}")
```

#### Advanced Python with Error Handling
```python
import requests
import json
import time
from typing import Dict, Any

class FlexTransmitter:
    def __init__(self, ip: str, port: int = 16180, 
                 username: str = "admin", password: str = "password"):
        self.base_url = f"http://{ip}:{port}/"
        self.auth = (username, password)
        self.session = requests.Session()
        
    def send_message(self, capcode: int, frequency: float, 
                    power: int, message: str, mail_drop: bool = False) -> Dict[str, Any]:
        """Send a FLEX paging message"""
        
        # Validate parameters
        if not (1 <= capcode <= 4294967295):
            raise ValueError("Capcode must be between 1 and 4,294,967,295")
        if not (400.0 <= frequency <= 1000.0):
            raise ValueError("Frequency must be between 400.0 and 1000.0 MHz")
        if not (0 <= power <= 20):
            raise ValueError("Power must be between 0 and 20 dBm")
        if not (1 <= len(message) <= 240):
            raise ValueError("Message must be 1-240 characters")
            
        data = {
            "capcode": capcode,
            "frequency": frequency,
            "power": power,
            "message": message,
            "mail_drop": mail_drop
        }
        
        try:
            response = self.session.post(
                self.base_url,
                auth=self.auth,
                headers={"Content-Type": "application/json"},
                json=data,
                timeout=10
            )
            
            if response.status_code == 200:
                return {"success": True, "data": response.json()}
            elif response.status_code == 401:
                return {"success": False, "error": "Authentication failed"}
            elif response.status_code == 503:
                return {"success": False, "error": "Device busy, try again later"}
            else:
                error_data = response.json() if response.content else {}
                return {"success": False, "error": error_data.get("error", "Unknown error")}
                
        except requests.exceptions.Timeout:
            return {"success": False, "error": "Request timeout"}
        except requests.exceptions.ConnectionError:
            return {"success": False, "error": "Connection failed"}
        except Exception as e:
            return {"success": False, "error": str(e)}

# Usage example
if __name__ == "__main__":
    transmitter = FlexTransmitter("192.168.1.100")
    
    result = transmitter.send_message(
        capcode=1234567,
        frequency=929.6625,
        power=10,
        message="Test from Python API",
        mail_drop=False
    )
    
    if result["success"]:
        print("Message sent successfully!")
        print(f"Response: {result['data']}")
    else:
        print(f"Error: {result['error']}")
```

### JavaScript/Node.js Examples

#### Node.js with axios
```javascript
const axios = require('axios');

async function sendFlexMessage(config) {
    const {ip, port = 16180, username = 'admin', password = 'password'} = config;
    const url = `http://${ip}:${port}/`;
    
    const data = {
        capcode: 1234567,
        frequency: 929.6625,
        power: 10,
        message: "Hello from Node.js!",
        mail_drop: false
    };
    
    try {
        const response = await axios.post(url, data, {
            auth: {username, password},
            headers: {'Content-Type': 'application/json'},
            timeout: 10000
        });
        
        console.log('Message sent successfully!');
        console.log(response.data);
        return {success: true, data: response.data};
        
    } catch (error) {
        if (error.response) {
            console.error(`Error ${error.response.status}:`, error.response.data);
            return {success: false, error: error.response.data};
        } else {
            console.error('Connection error:', error.message);
            return {success: false, error: error.message};
        }
    }
}

// Usage
sendFlexMessage({ip: '192.168.1.100'});
```

#### Browser JavaScript with fetch
```javascript
async function sendMessage() {
    const deviceIP = '192.168.1.100';
    const apiPort = 16180;
    const credentials = btoa('admin:password'); // Base64 encode
    
    const data = {
        capcode: 1234567,
        frequency: 929.6625,
        power: 10,
        message: document.getElementById('messageText').value,
        mail_drop: document.getElementById('mailDrop').checked
    };
    
    try {
        const response = await fetch(`http://${deviceIP}:${apiPort}/`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': `Basic ${credentials}`
            },
            body: JSON.stringify(data)
        });
        
        const result = await response.json();
        
        if (response.ok) {
            console.log('Success:', result);
            showStatus('Message sent successfully!', 'success');
        } else {
            console.error('Error:', result);
            showStatus(`Error: ${result.error}`, 'error');
        }
        
    } catch (error) {
        console.error('Connection error:', error);
        showStatus('Connection error', 'error');
    }
}

function showStatus(message, type) {
    const statusDiv = document.getElementById('status');
    statusDiv.textContent = message;
    statusDiv.className = `status ${type}`;
    statusDiv.style.display = 'block';
}
```

## 🔧 Integration Patterns

### Monitoring Systems

#### Nagios/Icinga Plugin
```bash
#!/bin/bash
# check_flex_transmitter.sh

DEVICE_IP="192.168.1.100"
API_PORT="16180"
USERNAME="admin"
PASSWORD="password"

# Test message
RESPONSE=$(curl -s -w "%{http_code}" \
  -u "$USERNAME:$PASSWORD" \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":1,"message":"Monitor test"}' \
  "http://$DEVICE_IP:$API_PORT/")

HTTP_CODE="${RESPONSE: -3}"

if [ "$HTTP_CODE" = "200" ]; then
    echo "OK - FLEX transmitter responding"
    exit 0
else
    echo "CRITICAL - FLEX transmitter not responding (HTTP $HTTP_CODE)"
    exit 2
fi
```

#### Prometheus Monitoring
```python
# flex_exporter.py
from prometheus_client import start_http_server, Counter, Histogram, Gauge
import requests
import time

# Metrics
messages_sent = Counter('flex_messages_sent_total', 'Total FLEX messages sent')
send_duration = Histogram('flex_send_duration_seconds', 'Time spent sending messages')
device_status = Gauge('flex_device_up', 'Device availability')

def check_device_status(ip, port, auth):
    """Check if device is responding"""
    try:
        response = requests.get(f"http://{ip}:{port}/status", 
                              auth=auth, timeout=5)
        return 1 if response.status_code == 200 else 0
    except:
        return 0

def send_test_message(ip, port, auth):
    """Send test message and measure duration"""
    data = {
        "capcode": 9999999,
        "frequency": 929.6625,
        "power": 1,
        "message": "Prometheus test"
    }
    
    start_time = time.time()
    try:
        response = requests.post(f"http://{ip}:{port}/", 
                               auth=auth, json=data, timeout=10)
        duration = time.time() - start_time
        
        if response.status_code == 200:
            messages_sent.inc()
            send_duration.observe(duration)
            return True
    except:
        pass
    return False

if __name__ == '__main__':
    start_http_server(8000)
    auth = ('admin', 'password')
    
    while True:
        device_status.set(check_device_status('192.168.1.100', 16180, auth))
        time.sleep(60)
```

### Home Automation

#### Home Assistant Integration
```yaml
# configuration.yaml
rest_command:
  send_flex_message:
    url: "http://192.168.1.100:16180/"
    method: POST
    username: admin
    password: password
    headers:
      Content-Type: "application/json"
    payload: >
      {
        "capcode": {{ capcode }},
        "frequency": 929.6625,
        "power": 10,
        "message": "{{ message }}",
        "mail_drop": {{ mail_drop | default(false) }}
      }

# automation.yaml
automation:
  - alias: "Send security alert via FLEX"
    trigger:
      platform: state
      entity_id: binary_sensor.front_door
      to: "on"
    condition:
      condition: state
      entity_id: alarm_control_panel.home
      state: "armed_away"
    action:
      service: rest_command.send_flex_message
      data:
        capcode: 1234567
        message: "Security Alert: Front door opened while away"
        mail_drop: true
```

## 🔒 Security Configuration

### Authentication Setup

Change default credentials immediately after deployment:

1. **Web Interface Method**:
   - Go to Configuration → API Settings
   - Set new username and password
   - Save configuration

2. **AT Command Method**:
   ```
   AT+APIUSER=newusername
   AT+APIPASS=newpassword
   ```

### Network Security

#### Reverse Proxy Setup (nginx)
```nginx
server {
    listen 443 ssl;
    server_name flex-api.example.com;
    
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;
    
    location / {
        proxy_pass http://192.168.1.100:16180;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header Authorization $http_authorization;
    }
}
```

#### Firewall Rules
```bash
# Allow API access only from specific IPs
iptables -A INPUT -p tcp --dport 16180 -s 192.168.1.0/24 -j ACCEPT
iptables -A INPUT -p tcp --dport 16180 -j DROP
```

## 📊 Rate Limiting & Performance

### API Limits
- **Concurrent Requests**: 1 (device processes one message at a time)
- **Message Queue**: None (requests rejected if device busy)
- **Timeout**: 10 seconds recommended for client timeout
- **Retry Logic**: Implement exponential backoff for 503 errors

### Performance Guidelines
- **Typical Response Time**: 100-500ms
- **Transmission Time**: 1-3 seconds depending on message length
- **Memory Usage**: ~2KB per API request
- **Concurrent Connections**: Limit to 5 simultaneous connections

### Error Handling Best Practices

```python
import time
import random

def send_with_retry(transmitter, max_retries=3):
    """Send message with exponential backoff retry"""
    for attempt in range(max_retries):
        result = transmitter.send_message(...)
        
        if result["success"]:
            return result
            
        if "busy" in result["error"].lower():
            # Exponential backoff for busy device
            delay = (2 ** attempt) + random.uniform(0, 1)
            time.sleep(delay)
            continue
        else:
            # Non-retryable error
            return result
    
    return {"success": False, "error": "Max retries exceeded"}
```

## 🧪 Testing & Validation

### API Testing Script
```bash
#!/bin/bash
# test_api.sh

API_URL="http://192.168.1.100:16180/"
AUTH="admin:password"

echo "Testing FLEX Transmitter API..."

# Test 1: Valid message
echo "Test 1: Valid message"
curl -s -u "$AUTH" -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":10,"message":"API Test"}' \
  "$API_URL" | jq .

# Test 2: Invalid power
echo -e "\nTest 2: Invalid power (should fail)"
curl -s -u "$AUTH" -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":25,"message":"Test"}' \
  "$API_URL" | jq .

# Test 3: Invalid frequency
echo -e "\nTest 3: Invalid frequency (should fail)"
curl -s -u "$AUTH" -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":2000,"power":10,"message":"Test"}' \
  "$API_URL" | jq .

# Test 4: Missing authentication
echo -e "\nTest 4: Missing authentication (should fail)"
curl -s -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"frequency":929.6625,"power":10,"message":"Test"}' \
  "$API_URL" | jq .

echo -e "\nAPI testing complete!"
```

---

**Related Documentation**: 
- [USER.md](USER.md) - Complete user interface guide
- [FIRMWARE.md](FIRMWARE.md) - Build and deployment guide
- [README.md](README.md) - Project overview