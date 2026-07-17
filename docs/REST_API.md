# FLEX Paging Message Transmitter - REST API Documentation

Complete REST API reference for programmatic control of the FLEX paging message transmitter.

> **Note**: this is a single-variant firmware — the REST API is present on every build (both TTGO LoRa32 and Heltec WiFi LoRa 32 V2), there is no separate "AT-only" build. For the serial AT command interface, see [AT_COMMANDS.md](AT_COMMANDS.md). For web interface usage, see [USER_GUIDE.md](USER_GUIDE.md).

## 🔗 API Overview

The FLEX paging message transmitter provides a RESTful HTTP API for remote message transmission and device control. The API uses JSON payloads and HTTP Basic Authentication for secure programmatic access.

### Supported Hardware
- **TTGO LoRa32**: full WiFi capabilities (ESP32 + SX1276)
- **Heltec WiFi LoRa 32 V2**: full WiFi capabilities (ESP32 + SX1276)

Both devices support identical API functionality and message lengths up to 248 characters.

### Base URL
```
http://<device-ip>/api
```

**Default Configuration**:
- **Port**: 80 (same as web interface, configurable via web interface settings)
- **Protocol**: HTTP (no HTTPS support)
- **Content-Type**: `application/json`
- **Endpoint**: `/api` for standard messages, `/api/v1/alerts` for Grafana webhooks, `/logs` for log retrieval, `/download_logs` for full log download

### Authentication
- **Method**: HTTP Basic Authentication
- **Factory-Default Credentials**: `admin` / `passw0rd` (`storage.cpp`) — change these before exposing the device
- **Header Format**: `Authorization: Basic <base64-encoded-credentials>`
- **Configuration**: Modify via the web interface only, on the separate `/api_config` page (there is no AT command for API credentials)
- **Not authenticated**: the WiFi management endpoints (`GET /api/wifi/scan`, `POST /api/wifi/add`, `POST /api/wifi/delete`) and the web-UI's own `POST /send` endpoint do **not** call the authentication check — only `/api` and `/api/v1/alerts` require Basic Auth. See [WiFi Management Endpoints](#wifi-management-endpoints-unauthenticated) and [Web UI Send Endpoint](#web-ui-send-endpoint-unauthenticated) below.

### Message Queue System
- **Queue Capacity**: Up to 25 concurrent message requests
- **Processing**: Automatic sequential transmission when device becomes idle
- **Queue Status**: Real-time feedback via HTTP response codes
- **Timeout**: 30 seconds per transmission

### Message Transmission Features

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

### Additional API Features

#### Grafana Integration (Webhook Endpoint)
- **Webhook URL**: `POST /api/v1/alerts` - Grafana Alertmanager compatible endpoint
- **Multi-Alert Support**: Single webhook call can process multiple alerts (body is a bare JSON array)
- **Queue Integration**: Grafana alerts use the same 25-message queue as other API calls
- **Configuration**: Enable/disable via web interface at `/grafana` or `settings.grafana_enabled`
- **Authentication**: Uses same HTTP Basic Auth as main API
- **Payload Format**: A bare JSON array of Grafana Alertmanager alert objects — see [Grafana Webhook Endpoint](#grafana-webhook-endpoint) below

## 📡 API Endpoints

### Send FLEX Message

**Endpoint**: `POST /api`

Transmits a FLEX paging message with specified parameters. Requires HTTP Basic Auth.

#### Request Format

```json
{
  "capcode": 1234567,
  "frequency": 931.9375,
  "power": 10,
  "message": "Your message text",
  "mail_drop": false
}
```

#### JSON Payload Attributes

| Attribute | Type | Required | Range/Format | Description |
|-----------|------|----------|--------------|-------------|
| `message` | string | ✅ | 1-248 characters (auto-truncated if longer) | Message text (ASCII printable chars) |
| `capcode` | integer | ❌ | any value (not range-checked by this endpoint) | Target FLEX capcode. Falls back to the device's default capcode (FLEX tab settings) if omitted |
| `frequency` | number | ❌ | 400.0 - 1000.0 | Transmission frequency in MHz. Falls back to the device's default frequency if omitted |
| `power` | integer | ❌ | 0 - 20 | Transmit power in dBm. Falls back to the device's default power if omitted |
| `tx_power` | integer | ❌ | 0 - 20 | Alternate field name accepted for `power` (checked only if `power` is absent) |
| `mail_drop` | boolean | ❌ | true/false | Mail drop flag (default: false) |

**Note**: unlike the web UI's own `/send` endpoint, `/api` does **not** validate the capcode against
FLEX's addressable ranges — any integer value is accepted and queued as-is. If you need range
validation, do it client-side, or see the `/send` endpoint below.

#### Frequency Format Support

The API accepts frequency in two formats with automatic conversion:

```json
// MHz format (recommended)
{"frequency": 931.9375}

// Hz format (auto-converted)
{"frequency": 929662500}
```

**Conversion Rule**: Values > 1000 are treated as Hz and divided by 1,000,000.

#### Response Format

`/api` always returns **HTTP 200** on success, regardless of whether the device transmits
immediately or the message is queued behind others — there is no 202 response from this endpoint
(only the separate `/send` endpoint distinguishes 200 vs 202).

**Immediate Transmission** (device idle, HTTP 200):
```json
{
  "frequency": 931.9375,
  "power": 10,
  "capcode": 1234567,
  "text": "Your message text",
  "truncated": false,
  "status": "queued",
  "message": "Message queued for immediate transmission"
}
```

**Queued Behind Other Messages** (device busy, HTTP 200):
```json
{
  "frequency": 931.9375,
  "power": 10,
  "capcode": 1234567,
  "text": "Your message text",
  "truncated": false,
  "status": "queued",
  "message": "Message queued for transmission",
  "queue_position": 3
}
```

**Truncated Message** (either case above adds `"truncated": true` and changes the `message` text to
mention truncation, e.g. `"Message truncated to 248 chars and queued for immediate transmission"`).

**Error Response** (HTTP 400/401/405/503) — flat shape:
```json
{
  "error": "Frequency must be between 400.0-1000.0 MHz or 400000000-1000000000 Hz"
}
```

**Queue Full** (HTTP 503) — a different, non-flat shape:
```json
{
  "status": "error",
  "message": "Queue is full. Please try again later.",
  "max_queue_size": 25
}
```

#### HTTP Status Codes

| Code | Status | Description |
|------|--------|-------------|
| 200 | OK | Message accepted and queued (immediately or behind others) — `/api` never returns 202 |
| 400 | Bad Request | Invalid/missing JSON, missing `message`, or frequency/power out of range |
| 401 | Unauthorized | Missing or invalid authentication |
| 405 | Method Not Allowed | Request method is not `POST` |
| 503 | Service Unavailable | API disabled (`{"error":"API service is disabled"}`), or device busy and queue full (25 messages) |

### Grafana Webhook Endpoint

**Endpoint**: `POST /api/v1/alerts`

Receives Grafana Alertmanager webhook notifications and converts them to FLEX paging messages.
Requires HTTP Basic Auth (same credentials as `/api`).

#### Request Format (Grafana Alertmanager)

The request body must be a **bare JSON array** of alert objects — not an object wrapping an
`"alerts"` key. `handle_grafana_webhook()` calls `doc.is<JsonArray>()` and rejects anything else
with `400 {"error":"Expected JSON array of alerts"}`.

```json
[
  {
    "labels": {
      "alertname": "HighCPUUsage",
      "capcode": "1234567"
    },
    "annotations": {
      "summary": "CPU usage exceeds 90%",
      "description": "Server web-01 CPU at 95%"
    },
    "endsAt": "0001-01-01T00:00:00Z"
  }
]
```

#### Grafana Alert Field Mapping

| Grafana Field | FLEX Mapping | Required | Description |
|--------------|--------------|----------|-------------|
| `endsAt` | FIRING/RESOLVED prefix | ❌ | `"0001-01-01T00:00:00Z"` (Alertmanager's "not yet resolved" sentinel) → `FIRING`; any other value → `RESOLVED`. There is no `status` field lookup. |
| `labels.alertname` | Alert name | ❌ | Falls back to `"Unknown Alert"` if missing |
| `labels.capcode` / `labels.pager_capcode` | Target capcode | ❌ | Falls back to the device's default capcode if both are missing |
| `labels.frequency` / `labels.pager_frequency` | Transmission frequency | ❌ | Falls back to the device's default frequency; MHz/Hz auto-conversion applies the same as `/api` |
| `labels.mail_drop` / `labels.pager_mail_drop` | Mail drop flag | ❌ | Default `false` |
| `annotations.summary` → `annotations.description` → `annotations.message` → `"Alert triggered"` | Message body | ❌ | First non-empty field in this fallback order wins |

Transmit power always uses the device's default power (`settings.default_txpower`) — there is no
per-alert power field. The final message text sent to the pager is formatted as
`"[FIRING] AlertName: message text"` (or `"[RESOLVED] ..."`), truncated to 248 characters with the
same ellipsis rule as `/api`.

#### Grafana Response Format

**Processed** (HTTP 200 if all alerts queued successfully, **HTTP 207** Multi-Status if any failed):
```json
{
  "status": "completed",
  "total_alerts": 1,
  "results": [
    {
      "alert_index": 1,
      "alert_name": "HighCPUUsage",
      "capcode": 1234567,
      "frequency": 931.9375,
      "message": "[FIRING] HighCPUUsage: CPU usage exceeds 90%",
      "truncated": false,
      "success": true
    }
  ],
  "successful": 1,
  "failed": 0
}
```

A failed entry (queue full while processing that alert) sets `"success": false` and adds an
`"error": "Queue is full"` field on that result object, without aborting the rest of the batch.

**Grafana Disabled** (HTTP 503):
```json
{
  "error": "Grafana webhook service is disabled"
}
```

**Partial Failure — Some Alerts Queued, Some Failed** (HTTP 207, not 503):
```json
{
  "status": "completed",
  "total_alerts": 5,
  "results": ["... 5 entries, some with \"success\": false ..."],
  "successful": 2,
  "failed": 3
}
```

503 from this endpoint is reserved for `grafana_enabled == false` — a full queue never produces a
503 here, only 207.

#### Grafana Configuration Example

```yaml
# alertmanager.yml
receivers:
  - name: 'flex-pager'
    webhook_configs:
      - url: 'http://192.168.1.100/api/v1/alerts'
        http_config:
          basic_auth:
            username: 'admin'
            password: 'passw0rd'
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

Alertmanager's own webhook body wraps alerts in `{"alerts": [...]}` — this firmware does **not**
accept that wrapper. If your notification pipeline can't emit a bare array natively, insert a
transform step (e.g. an Alertmanager `--web.templates`-based proxy, or a small middleware) that
unwraps `.alerts` before forwarding to this device.

### WiFi Management Endpoints (unauthenticated)

These three endpoints manage the device's stored WiFi network list and are registered without any
call to `authenticate_api_request()` — no HTTP Basic Auth is required or checked, unlike `/api` and
`/api/v1/alerts`.

**`GET /api/wifi/scan`** — triggers a WiFi scan (or returns results of one already running) and
returns `{"success":true,"networks":[{"ssid":...,"rssi":...,"channel":...,"encryption":"Open"|"Encrypted","stored":true|false}, ...]}`.
Returns HTTP 202 with `{"success":false,"scanning":true}` if a scan is already in progress, or
HTTP 500 on scan failure/timeout.

**`POST /api/wifi/add`** — form-encoded (not JSON) body with `ssid`, `password`, optional
`use_dhcp`, `static_ip`, `netmask`, `gateway`, `dns`. Adds or updates a stored network entry.
Returns `{"success":true}` (HTTP 200) or `{"success":false,"error":"..."}` (HTTP 400/500).

**`POST /api/wifi/delete`** — form-encoded body with `ssid`. Removes a stored network entry.
Returns `{"success":true,"message":"Network deleted"}` (HTTP 200), or
`{"success":false,"message":"Network not found"}` (HTTP 404) / `{"success":false,"message":"Missing SSID parameter"}` (HTTP 400).

### Web UI Send Endpoint (unauthenticated)

**Endpoint**: `POST /send`

This is the web interface's own send path (used by the `/` page's message form), separate from
`/api`. It is unauthenticated, form-encoded (not JSON), and unlike `/api` it **does** validate the
capcode range.

Required form fields: `frequency`, `power`, `capcode`, `message`. Optional: `mail_drop` (presence
of the field, any value, counts as true).

**Capcode validation**: `validate_flex_capcode()` (`flex_protocol.cpp`) accepts
`1-1933312, 1998849-2031614, 2101249-4297068542` and rejects everything else with HTTP 400
`{"success":false,"message":"..."}`. This is a materially different (and stricter) check than
`/api`, which performs no capcode validation at all.

**Known cosmetic bug in the rejection message**: the error text baked into `/send` (and two
ChatGPT capcode fields) literally prints `"Valid ranges: 1-1933312, 1998849-2031614,
2101249-4291000000"` — a stale upper bound left over from before firmware `v3.8.71` raised the
real ceiling to `4297068542`. The validation logic already uses the correct, higher bound; only
the printed string undersells it. A capcode like `4295000000` is valid and will transmit
successfully even though it falls outside the range the error message would claim.

**Response codes**: **200** if the device was idle (immediate transmission) or **202** if queued
behind other messages — a real 200/202 split, unlike `/api`. Response shape is
`{"success":true,"message":"..."}` (not `{"status":...}`). Returns 503 with
`{"success":false,"message":"Queue is full. Please try again later."}` if the queue is full.

### Retrieve Device Logs

**Endpoint**: `GET /logs`

Returns recent log lines from the persistent SPIFFS log file as a JSON array of objects.
Unauthenticated.

#### Query Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `lines` | integer | ❌ | 20 | Number of log lines to return |

#### Request Example

```bash
# Get last 20 log lines (default)
curl -s http://DEVICE_IP/logs

# Get last 100 log lines
curl -s http://DEVICE_IP/logs?lines=100
```

#### Response Format (always HTTP 200)

Each entry is an object with separate `timestamp` and `message` fields (split from each raw log
line at the 20-character timestamp prefix), not a plain string:

```json
{
  "logs": [
    {"timestamp": "2026-02-16 10:30:45", "message": "SYSTEM: Boot complete"},
    {"timestamp": "2026-02-16 10:30:46", "message": "WIFI: Connected to MyNetwork (192.168.1.100)"},
    {"timestamp": "2026-02-16 10:31:00", "message": "TX: Message sent to capcode 1234567"}
  ]
}
```

`handle_logs()` always returns HTTP 200, even when no log file exists yet (in which case `"logs"`
is simply an empty array) — there is no 404 case for this endpoint.

### Download Full Log File

**Endpoint**: `GET /download_logs`

Downloads the complete `/serial.log` file from SPIFFS. Unauthenticated.

```bash
curl -s http://DEVICE_IP/download_logs -o serial.log
```

**Response**: Raw text file with `Content-Disposition: attachment; filename="serial.log"` (HTTP
200), or `404 "Log file not found"` (plain text, not JSON) if `/serial.log` doesn't exist yet.

**Log File Specifications**:
- **File**: `/serial.log` on SPIFFS
- **Max Size**: 64KB (`MAX_LOG_FILE_SIZE`, `config.h`) — auto-truncates, keeping the last 32KB (`LOG_TRUNCATE_SIZE`)
- **Timestamp Format (pre-NTP/RTC)**: `0000-00-00 HH:MM:SS`
- **Timestamp Format (post-NTP/RTC)**: `YYYY-MM-DD HH:MM:SS`
- **Order**: Chronological (oldest → newest)

---

## 🔧 Programming Examples

### cURL Examples

**Basic Message Transmission**:
```bash
curl -X POST http://192.168.1.100/api \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 931.9375,
    "power": 10,
    "message": "Hello from REST API"
  }'
```

**Message with Mail Drop Flag**:
```bash
curl -X POST http://192.168.1.100/api \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 931.9375,
    "power": 15,
    "message": "Urgent notification",
    "mail_drop": true
  }'
```

**Omitting capcode/frequency/power (falls back to device defaults)**:
```bash
curl -X POST http://192.168.1.100/api \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"message": "Uses device default capcode/frequency/power"}'
```

**Using Hz Frequency Format**:
```bash
curl -X POST http://192.168.1.100/api \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1234567,
    "frequency": 929662500,
    "power": 10,
    "message": "Hz format test"
  }'
```

**Test Grafana Webhook** (bare array body):
```bash
curl -X POST http://192.168.1.100/api/v1/alerts \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '[
    {
      "labels": {
        "alertname": "TestAlert",
        "capcode": "1234567"
      },
      "annotations": {
        "summary": "This is a test alert from curl"
      },
      "endsAt": "0001-01-01T00:00:00Z"
    }
  ]'
```

### Python Integration

**Simple Message Sending**:
```python
import requests
import json

def send_flex_message(device_ip, capcode, message, frequency=931.9375, power=10, mail_drop=False,
                       username="admin", password="passw0rd"):
    url = f"http://{device_ip}/api"

    payload = {
        "capcode": capcode,
        "frequency": frequency,
        "power": power,
        "message": message,
        "mail_drop": mail_drop
    }

    try:
        response = requests.post(
            url,
            json=payload,
            auth=(username, password),
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
    def __init__(self, device_ip, username="admin", password="passw0rd", port=80):
        self.base_url = f"http://{device_ip}:{port}/api"
        self.grafana_url = f"http://{device_ip}:{port}/api/v1/alerts"
        self.auth = (username, password)
        self.session = requests.Session()
        self.session.auth = self.auth

    def send_message(self, capcode=None, message="", frequency=None, power=None, mail_drop=False):
        """Send FLEX message. capcode/frequency/power are optional — omit to use device defaults."""

        if frequency is not None and not (400.0 <= frequency <= 1000.0):
            raise ValueError("Frequency must be between 400.0 and 1000.0 MHz")
        if power is not None and not (0 <= power <= 20):
            raise ValueError("Power must be between 0 and 20 dBm")
        # Note: /api does not validate the capcode range at all.
        # Note: Messages longer than 248 characters are auto-truncated by firmware

        payload = {"message": message, "mail_drop": mail_drop}
        if capcode is not None:
            payload["capcode"] = capcode
        if frequency is not None:
            payload["frequency"] = frequency
        if power is not None:
            payload["power"] = power

        try:
            response = self.session.post(self.base_url, json=payload, timeout=30)
            return self._handle_response(response)
        except requests.exceptions.Timeout:
            raise Exception("Request timeout - device may be busy")
        except requests.exceptions.ConnectionError:
            raise Exception("Connection failed - check device IP and network")

    def send_grafana_alerts(self, alerts):
        """Send Grafana-formatted alerts to webhook endpoint. `alerts` must be a list — the
        request body is that bare list, not wrapped in an 'alerts' key."""

        try:
            response = self.session.post(self.grafana_url, json=alerts, timeout=30)
            return self._handle_response(response)
        except requests.exceptions.Timeout:
            raise Exception("Grafana webhook timeout")
        except requests.exceptions.ConnectionError:
            raise Exception("Connection failed - check device IP and network")

    def _handle_response(self, response):
        """Handle API response and extract data"""
        data = response.json()

        if response.status_code in [200, 207]:
            return {
                "success": True,
                "status_code": response.status_code,
                "data": data
            }
        else:
            raise Exception(f"API Error ({response.status_code}): {data.get('error', data.get('message', 'Unknown error'))}")

# Usage - Standard Message
api = FlexAPI("192.168.1.100", "admin", "passw0rd")

try:
    result = api.send_message(capcode=1234567, message="Advanced Python client test", power=15, mail_drop=True)
    print(f"Message sent successfully: {result['data']}")
except Exception as e:
    print(f"Error: {e}")

# Usage - Grafana Alert (bare array, no "alerts" wrapper)
try:
    alerts = [
        {
            "labels": {
                "alertname": "HighMemory",
                "capcode": "1234567"
            },
            "annotations": {
                "summary": "Memory usage exceeds 90%"
            },
            "endsAt": "0001-01-01T00:00:00Z"
        }
    ]
    result = api.send_grafana_alerts(alerts)
    print(f"Grafana alerts processed: {result['data']}")
except Exception as e:
    print(f"Grafana error: {e}")
```

### JavaScript/Node.js Integration

```javascript
const axios = require('axios');

class FlexAPI {
    constructor(deviceIP, username = 'admin', password = 'passw0rd', port = 80) {
        this.baseURL = `http://${deviceIP}:${port}/api`;
        this.grafanaURL = `http://${deviceIP}:${port}/api/v1/alerts`;
        this.auth = {
            username: username,
            password: password
        };
    }

    async sendMessage(message, options = {}) {
        // capcode/frequency/power are optional — omitted fields fall back to device defaults
        const payload = { message: message, mail_drop: options.mail_drop || false };
        if (options.capcode !== undefined) payload.capcode = options.capcode;
        if (options.frequency !== undefined) payload.frequency = options.frequency;
        if (options.power !== undefined) payload.power = options.power;

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

    async sendGrafanaAlerts(alerts) {
        // Body must be the bare array itself, not { alerts: [...] }
        try {
            const response = await axios.post(this.grafanaURL, alerts, {
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
    const api = new FlexAPI('192.168.1.100', 'admin', 'passw0rd');

    try {
        const result = await api.sendMessage('Node.js API test', {
            capcode: 1234567,
            frequency: 931.9375,
            power: 15,
            mail_drop: true
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
    const api = new FlexAPI('192.168.1.100', 'admin', 'passw0rd');

    const alerts = [
        {
            labels: {
                alertname: 'DiskSpaceLow',
                capcode: '1234567'
            },
            annotations: {
                summary: 'Disk space below 10%'
            },
            endsAt: '0001-01-01T00:00:00Z'
        }
    ];

    try {
        const result = await api.sendGrafanaAlerts(alerts);

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
API_PORT="80"
USERNAME="admin"
PASSWORD="passw0rd"

# Function to send FLEX message
send_flex_message() {
    local capcode=$1
    local message=$2
    local frequency=${3:-931.9375}
    local power=${4:-10}
    local mail_drop=${5:-false}

    local payload=$(cat <<EOF
{
    "capcode": $capcode,
    "frequency": $frequency,
    "power": $power,
    "message": "$message",
    "mail_drop": $mail_drop
}
EOF
)

    curl -s -X POST "http://$DEVICE_IP:$API_PORT/api" \
        -u "$USERNAME:$PASSWORD" \
        -H "Content-Type: application/json" \
        -d "$payload" \
        --max-time 30
}

# Function to send Grafana alerts (bare array body, not wrapped in "alerts")
send_grafana_alerts() {
    local capcode=$1
    local alert_name=$2
    local summary=$3

    local payload=$(cat <<EOF
[
    {
        "labels": {
            "alertname": "$alert_name",
            "capcode": "$capcode"
        },
        "annotations": {
            "summary": "$summary"
        },
        "endsAt": "0001-01-01T00:00:00Z"
    }
]
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
send_flex_message 1234567 "Urgent alert" 931.9375 15 true

echo -e "\nSending Grafana alert..."
send_grafana_alerts 1234567 "HighCPU" "CPU usage exceeds 90%"
```

## ⚙️ API Configuration

### Authentication Management

**Set Custom Credentials via the Web Interface** (there is no AT command for this — `AT+APIUSER`/`AT+APIPASS` do not exist in this firmware):
1. Navigate to `http://<device-ip>/api_config`
2. Enable the API toggle, set username/password
3. Save — settings apply immediately, no separate save step

**Check API status over AT commands**: `AT+DEVICE?` includes a `+DEVICE_API: Enabled|Disabled`
line, but does not report the username/password.

**Configure HTTP Port**: Use the web interface (Configuration page) to change the HTTP server port (default: 80). The API and web interface share the same port.

### Network Configuration

**Configure WiFi via AT Commands**:
```bash
# Connect to WiFi network
AT+WIFI=YourNetwork,YourPassword

# Check connection status
AT+WIFI?
# Response: +WIFI: CONNECTED,192.168.1.100
```

### Grafana Integration Configuration

**Enable/Disable Grafana Webhook**: there is no AT command for this (`AT+GRAFANA` does not exist)
— use the web interface only.

**Check Grafana status over AT commands**:
```bash
AT+DEVICE?
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
  "error": "Authentication required"
}
```

**Parameter Validation Error (400)**:
```json
{
  "error": "Frequency must be between 400.0-1000.0 MHz or 400000000-1000000000 Hz"
}
```

**Queue Full (503)**:
```json
{
  "status": "error",
  "message": "Queue is full. Please try again later.",
  "max_queue_size": 25
}
```

**API Service Disabled (503)**:
```json
{
  "error": "API service is disabled"
}
```

**Grafana Webhook Disabled (503)**:
```json
{
  "error": "Grafana webhook service is disabled"
}
```

Every `/api` and `/api/v1/alerts` error body except the queue-full case above is a flat
`{"error": "..."}` object — there is no nested `"details"` field and no separate `"status"` key on
those responses.

### Quick Troubleshooting

1. **Connection Issues**: Test connectivity with `ping` and verify port accessibility
2. **Authentication Issues**: Verify credentials on the web interface's `/api_config` page (there is no AT command to read them back)
3. **Parameter Validation**: `/api` only validates frequency (400-1000 MHz) and power (0-20 dBm) — capcode is accepted unchecked. Message length is auto-truncated at 248 characters
4. **Grafana Issues**: Verify Grafana is enabled via the web interface, or check `AT+DEVICE?`'s `+DEVICE_GRAFANA` line. Confirm the webhook body is a bare array, not `{"alerts": [...]}`
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
- **Power**: 0 to +20 dBm (REST API `power` field range)
- **Serial Port**: `/dev/ttyACM0` (Linux), `COM3+` (Windows)
- **Default Frequency**: 931.9375 MHz
- **Display**: 128x64 OLED (I2C)
- **WiFi**: 802.11 b/g/n (2.4GHz)
- **Message Limit**: 248 characters

### Heltec WiFi LoRa 32 V2
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio**: SX1276 (433/868/915 MHz)
- **Power**: 0 to +20 dBm (REST API `power` field range)
- **Serial Port**: `/dev/ttyUSB0` (Linux), `COM4+` (Windows)
- **Default Frequency**: 931.9375 MHz
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

## 🔧 API Versioning

- **API Version**: v1
- **Endpoints**: `/api` (standard messages), `/api/v1/alerts` (Grafana webhooks), `/logs`, `/download_logs`, plus the unauthenticated `/api/wifi/scan`, `/api/wifi/add`, `/api/wifi/delete`, and `/send`

This is a single-variant firmware - every documented endpoint and feature above is present
on every build (both TTGO LoRa32 and Heltec WiFi LoRa 32 V2). There is no separate AT-only
build; for serial control instead of HTTP, see [AT_COMMANDS.md](AT_COMMANDS.md).

## 🆕 Feature Details

### Grafana Webhook Integration
- **Endpoint**: `POST /api/v1/alerts`
- **Purpose**: Receive Grafana Alertmanager notifications and convert to FLEX pages
- **Configuration**: Web interface at `/grafana` with enable/disable toggle
- **Alert Mapping**: Automatic extraction of capcode from labels, message from annotations
- **Multi-Alert**: Process multiple alerts in a single webhook call (bare JSON array body)
- **Queue Integration**: Uses same 25-message queue as standard API

### Message Queue
- **Capacity**: Up to 25 messages
- **Benefit**: Handles burst traffic from monitoring systems
- **Status**: Queue position included in `/api` responses once a message is behind others
- **Full Queue Handling**: Returns 503 on `/api`/`/send`; returns 207 with `failed > 0` on `/api/v1/alerts`

### Device Discovery
- **OLED Display**: Shows IP address and connection status
- **AT Commands**: Status reporting via `AT+DEVICE?`
- **Web Interface**: Configuration pages show current network settings
