# Changelog

All notable changes to the flex-fsk-tx project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.2.0] - 2025-08-27

### Added
- Message queue system supporting up to 10 concurrent requests
- Automatic sequential transmission processing
- HTTP 202 (Accepted) response for queued messages
- Queue status feedback via API and web interface
- Elimination of "device busy" errors for concurrent access

### Changed
- REST API now returns HTTP 202 for queued messages instead of HTTP 409 for busy device
- Multi-user experience significantly improved with automatic queuing
- HTTP 503 (Service Unavailable) now indicates queue full instead of device busy

### Technical Details
- Queue capacity: 10 messages maximum
- Automatic processing when device becomes idle
- Queue position feedback in API responses

## [3.1.0] - 2025-08-26

### Added
- Enhanced AP mode display with clear connection information
- Consistent SSID generation with standardized 4-character hex format
- PPM correction feature for TTGO devices (all firmware versions)
- Display optimization with better font management
- Periodic display refresh in AP mode

### Changed
- AP mode display now shows "AP Mode Active", SSID, password, and IP address
- SSID format standardized: TTGO_FLEX_XXXX and HELTEC_FLEX_XXXX (4 hex chars)
- Improved OLED positioning and font management
- Battery information moved out of AP mode display for cleaner UX

### Deprecated
- Heltec WiFi LoRa 32 V3 devices due to SX1262 chipset limitations (130 character limit)

## [3.0.0] - 2024-XX-XX

### Added
- v3 firmware with comprehensive WiFi capabilities
- Professional web interface for message transmission
- REST API with HTTP Basic Authentication
- Standalone operation mode (no host computer required)
- Configuration portal for device settings
- Multiple theme support (Default/Blue, Light, Dark)
- EEPROM-based configuration storage
- Multi-user concurrent access support

### Changed
- Major architecture shift to standalone WiFi operation
- Web interface becomes primary control method
- Enhanced AT command set for WiFi management

## [2.0.0] - 2024-XX-XX

### Added
- v2 firmware with on-device FLEX encoding
- AT+MSG command for simplified message transmission
- Mail drop flag support (AT+MAILDROP)
- Remote encoding mode reduces host application dependencies

### Changed
- Dual encoding modes: local (host PC) and remote (device)
- Enhanced AT command protocol

## [1.0.0] - 2024-XX-XX

### Added
- Initial release with basic AT command interface
- v1 firmware supporting binary transmission
- TTGO LoRa32-OLED and Heltec WiFi LoRa 32 V3 hardware support
- Host application for FLEX message encoding
- Serial communication at 115200 baud
- Basic radio configuration (frequency, power)
- OLED status display
- tinyflex library integration for FLEX protocol

### Features
- Binary data transmission via AT+SEND
- Frequency range: 400-1000 MHz (hardware dependent)
- Power range: 0-20 dBm (TTGO), -9 to +22 dBm (Heltec)
- Cross-platform host application (Linux, macOS, Unix)

---

## Versioning Guidelines

This project follows [Semantic Versioning](https://semver.org/):

- **MAJOR** version: Incompatible API changes, breaking changes
- **MINOR** version: New functionality in backwards-compatible manner
- **PATCH** version: Backwards-compatible bug fixes

### Version Increment Rules

**MAJOR increments**:
- Breaking AT command protocol changes
- Incompatible REST API changes
- Major hardware platform changes
- Fundamental architecture changes

**MINOR increments**:
- New firmware versions (v1 → v2 → v3)
- New interface additions (web interface, REST API)
- Major new features (message queue, WiFi support)
- New AT commands or API endpoints
- New hardware support

**PATCH increments**:
- Bug fixes and stability improvements
- Documentation updates
- UI/UX improvements
- Performance optimizations
- Configuration enhancements