# flex-fsk-tx

**Professional FLEX Paging Message Transmitter System for ESP32 LoRa32 Devices**

A comprehensive, feature-rich solution for transmitting FLEX pager messages using ESP32 LoRa32 development boards. This project provides multiple control interfaces, encoding methods, and operation modes to meet diverse paging transmission requirements.

---

## 🎯 Project Scope and Vision

**flex-fsk-tx** transforms ESP32 LoRa32 development boards into powerful, professional-grade FLEX paging message transmitters. Whether you're a ham radio operator, system integrator, hobbyist, or business user, this project provides enterprise-level features with the simplicity of consumer electronics.

### Key Mission
- **Democratize FLEX Paging**: Make professional paging technology accessible to everyone
- **Hardware Flexibility**: Support multiple ESP32 LoRa32 platforms with unified firmware
- **Interface Diversity**: Provide command-line, web, and API access methods
- **Professional Quality**: Enterprise-grade reliability with comprehensive error handling
- **Community Driven**: Open source with extensive documentation and support

---

## 🚀 Device Capabilities Overview

### Multiple Operation Modes
**flex-fsk-tx** devices can operate in several distinct modes to suit different use cases:

#### 1. **Host-Controlled Mode** (v1/v2 Firmware)
- **Serial AT Command Interface**: Direct communication via USB serial port
- **C++ Host Application**: Computer-based control with advanced scripting capabilities
- **Local Encoding**: FLEX messages encoded on host computer using tinyflex library
- **Remote Encoding**: On-device FLEX encoding for simplified host applications
- **Batch Processing**: Multiple message transmission with loop mode support

#### 2. **Standalone WiFi Mode** (v3 Firmware)
- **Web Browser Interface**: Point-and-click message transmission from any device
- **REST API**: HTTP JSON API for system integration and automation
- **Independent Operation**: No host computer required for basic operation
- **Configuration Portal**: Complete device setup via web interface
- **Multi-User Access**: Simultaneous access from multiple devices

#### 3. **Hybrid Mode** (v3 Firmware)
- **All Interfaces Available**: AT commands, web interface, and REST API simultaneously
- **Flexible Control**: Choose the best interface for each task
- **Seamless Integration**: Legacy AT command compatibility with modern web features

### Advanced Device Features

#### **Transmission Capabilities**
- **FLEX Protocol**: Complete implementation of FLEX paging standard
- **Wide Frequency Range**: 400-1000 MHz (hardware dependent)
- **Variable Power Output**: -9 to +22 dBm adjustable transmission power
- **Mail Drop Support**: Priority message flagging for store-and-forward systems
- **Message Validation**: Real-time parameter checking and error prevention
- **Multiple Message Formats**: Binary data transmission and text message encoding

#### **Hardware Integration**
- **Dual Platform Support**: Optimized for both TTGO LoRa32-OLED and Heltec WiFi LoRa 32 V3
- **Radio Chipset Control**: Direct SX1276/SX1262 chipset optimization
- **OLED Status Display**: Real-time system status and transmission feedback
- **LED Indicators**: Visual feedback for system states and operations
- **Power Management**: Intelligent display timeout and power saving features
- **Antenna Safety**: Built-in protection against transmission without antenna

#### **Network and Connectivity** (v3 Firmware)
- **WiFi Station Mode**: Connect to existing networks with DHCP or static IP
- **Access Point Mode**: Create hotspot for direct device configuration
- **HTTP Web Server**: Full-featured web interface on port 80
- **REST API Server**: JSON API with HTTP Basic Authentication on configurable port
- **mDNS Support**: Easy device discovery on local networks
- **Network Security**: WPA2 WiFi security with configurable API authentication

#### **Configuration and Management**
- **EEPROM Storage**: Persistent configuration storage with factory reset capability
- **Theme Support**: Multiple UI themes (Default/Blue, Light, Dark) with real-time switching
- **Custom Branding**: Configurable device banner and identification
- **Settings Backup**: Configuration export and import capabilities
- **Over-the-Air Updates**: Future firmware update capabilities via web interface
- **System Monitoring**: Battery voltage, uptime, memory usage, and temperature monitoring

#### **Developer and Integration Features**
- **AT Command Protocol**: Standardized Hayes-compatible command set
- **Multiple Encoding Modes**: Host-side and device-side FLEX encoding options
- **Serial Communication**: 115200 baud rate with comprehensive error handling
- **JSON API**: RESTful API with parameter validation and detailed error responses
- **Rate Limiting**: Built-in transmission queue management
- **Status Reporting**: Comprehensive device state and health monitoring
- **Error Recovery**: Automatic retry logic with exponential backoff

---

## 🔧 Supported Hardware Platforms

### Primary Supported Devices

#### **TTGO LoRa32-OLED** (LilyGO)
- **MCU**: ESP32 (240MHz dual-core Xtensa LX6)
- **Radio**: Semtech SX1276 LoRa/FSK transceiver
- **Display**: 0.96" OLED (128x64 pixels, SSD1306)
- **Connectivity**: USB-C, WiFi 802.11 b/g/n, Bluetooth
- **Power Range**: 0-20 dBm configurable transmission power
- **Frequency Bands**: 433/868/915 MHz (region dependent)
- **Serial Interface**: Typically `/dev/ttyACM0` on Linux, COM ports on Windows
- **Special Features**: Integrated battery management, compact form factor
- **Firmware Compatibility**: Full v1/v2/v3 firmware support with web interface

#### **Heltec WiFi LoRa 32 V3** (Heltec Automation)
- **MCU**: ESP32-S3 (240MHz dual-core Xtensa LX7)
- **Radio**: Semtech SX1262 LoRa/FSK transceiver (next-generation chipset)
- **Display**: 0.96" OLED (128x64 pixels, SSD1306)
- **Connectivity**: USB-C, WiFi 802.11 b/g/n, Bluetooth 5.0
- **Power Range**: -9 to +22 dBm extended power range
- **Frequency Bands**: 410-1000 MHz extended frequency range
- **Serial Interface**: Typically `/dev/ttyUSB0` on Linux, COM ports on Windows
- **Special Features**: Enhanced power management, extended frequency range
- **Firmware Compatibility**: Full v1/v2 firmware support (v3 under development)

### Hardware Acquisition

#### **TTGO LoRa32-OLED**
- **Primary Sources**: AliExpress, Banggood, Amazon
- **Regional Availability**: Global shipping available
- **Price Range**: $15-25 USD (varies by supplier and region)
- **Verification**: Ensure OLED display is included for full functionality

#### **Heltec WiFi LoRa 32 V3**
- **Official Store**: [Heltec Automation](https://heltec.org/)
- **Authorized Distributors**: Digi-Key, Mouser, Arrow Electronics
- **Regional Availability**: Professional electronics distributors worldwide
- **Price Range**: $20-30 USD (professional distribution channels)
- **Verification**: Confirm V3 variant (ESP32-S3 + SX1262)

---

## 📡 Firmware Architecture and Versions

### Firmware Evolution

#### **v1 Firmware: Foundation**
**Design Philosophy**: Simple, reliable, minimal resource usage
- **Local Encoding**: FLEX messages encoded on host computer using tinyflex library
- **Binary Transmission**: Raw data transmission via `AT+SEND` command
- **AT Command Interface**: Basic Hayes-compatible command set
- **Memory Efficiency**: Minimal RAM and flash usage for resource-constrained applications
- **Host Application Dependency**: Requires flex-fsk-tx host application for FLEX encoding
- **Target Users**: Developers, system integrators, resource-conscious applications

#### **v2 Firmware: Enhanced**
**Design Philosophy**: Device intelligence, reduced host dependencies
- **All v1 Features**: Complete backward compatibility maintained
- **Remote Encoding**: On-device FLEX encoding using embedded tinyflex library
- **Dual Operation Modes**: Support both local and remote encoding methods
- **Enhanced AT Commands**: Additional commands for mail drop and message transmission
- **Simplified Integration**: Host applications can send plain text instead of binary data
- **Target Users**: Application developers, automated systems, simplified integrations

#### **v3 Firmware: Professional**
**Design Philosophy**: Standalone operation, enterprise features, user accessibility
- **All v2 Features**: Complete AT command and encoding compatibility
- **WiFi Connectivity**: Full 802.11 b/g/n support with multiple operation modes
- **Web Interface**: Professional browser-based control interface
- **REST API**: Complete HTTP JSON API for system integration
- **Standalone Operation**: Independent message transmission without host computer
- **Advanced Configuration**: EEPROM-based persistent settings management
- **Multi-User Support**: Concurrent access from multiple clients
- **Professional UI**: Theme support, real-time validation, responsive design
- **Target Users**: End users, business applications, IoT integration, professional deployments

### Firmware Selection Guide

| Feature | v1 Firmware | v2 Firmware | v3 Firmware |
|---------|-------------|-------------|-------------|
| **AT Commands** | ✅ Basic | ✅ Enhanced | ✅ Complete |
| **Local Encoding** | ✅ Host PC | ✅ Host PC | ✅ Host PC |
| **Remote Encoding** | ❌ | ✅ Device | ✅ Device |
| **WiFi Connectivity** | ❌ | ❌ | ✅ Full |
| **Web Interface** | ❌ | ❌ | ✅ Professional |
| **REST API** | ❌ | ❌ | ✅ Complete |
| **Standalone Operation** | ❌ | ❌ | ✅ Full |
| **Memory Usage** | Minimal | Moderate | High |
| **Configuration Storage** | ❌ | ❌ | ✅ EEPROM |
| **Multi-User Access** | ❌ | ❌ | ✅ Concurrent |
| **Theme Support** | ❌ | ❌ | ✅ Multiple |

---

## 🌐 Interface Ecosystem

### 1. **Command Line Interface** (All Firmware Versions)
**Target Users**: Developers, system administrators, automation systems

#### **AT Command Protocol**
- **Hayes Compatibility**: Industry-standard AT command format
- **Comprehensive Command Set**: Device configuration, transmission control, status monitoring
- **Parameter Validation**: Real-time input validation with detailed error messages
- **State Management**: Intelligent device state tracking and recovery
- **Error Handling**: Automatic retry logic with exponential backoff
- **Documentation**: Complete command reference in [AT_COMMANDS.md](AT_COMMANDS.md)

#### **Host Application** (C++)
- **Cross-Platform**: Linux, macOS, Unix compatibility
- **Multiple Input Modes**: Command line arguments, stdin, loop mode
- **Encoding Options**: Local tinyflex encoding or remote device encoding
- **Batch Processing**: Multiple message transmission with queue management
- **Error Recovery**: Comprehensive timeout and retry mechanisms
- **Build System**: Simple Makefile-based compilation and installation

### 2. **Web Interface** (v3 Firmware Only)
**Target Users**: End users, occasional users, non-technical operators

#### **Main Transmission Interface**
- **Intuitive Design**: Point-and-click message transmission
- **Real-Time Validation**: Instant parameter checking and error highlighting
- **Character Counter**: Live message length tracking
- **Frequency Helper**: Common frequency presets and validation
- **Power Guidelines**: Safe power level recommendations
- **Transmission Feedback**: Real-time status updates and confirmation

#### **Configuration Portal**
- **WiFi Management**: Network connection and credentials management
- **Device Settings**: Custom banners, API configuration, system preferences
- **Theme Selection**: Multiple UI themes with real-time preview
- **System Information**: Hardware status, uptime, memory usage
- **Factory Reset**: Safe configuration reset with confirmation dialogs

#### **Status Dashboard**
- **System Health**: Real-time device monitoring and diagnostics
- **Network Status**: WiFi connectivity and IP address information
- **Battery Monitoring**: Voltage levels and estimated battery life
- **Transmission History**: Recent message transmission log
- **Error Reporting**: Comprehensive error tracking and resolution guidance

### 3. **REST API** (v3 Firmware Only)
**Target Users**: Developers, system integrators, automated systems

#### **HTTP JSON API**
- **RESTful Design**: Standard HTTP methods and status codes
- **JSON Payload**: Structured data format for easy integration
- **Parameter Validation**: Server-side input validation with detailed error responses
- **Authentication**: HTTP Basic Auth with configurable credentials
- **Rate Limiting**: Built-in transmission queue management
- **Documentation**: Complete API reference in [REST_API.md](REST_API.md)

#### **Integration Examples**
- **Home Automation**: Integration with smart home systems
- **Business Applications**: Automated notification systems
- **IoT Platforms**: Sensor-triggered messaging
- **Monitoring Systems**: Alert and alarm transmission
- **Custom Applications**: Direct API integration in any programming language

---

## 🔬 Technical Specifications

### **Radio Performance**
- **Transmission Protocol**: FLEX (Forward Link EXchange) paging standard
- **Modulation**: FSK (Frequency Shift Keying)
- **Data Rate**: 1.6 kbps (FLEX standard)
- **Frequency Deviation**: 5 kHz
- **Receive Bandwidth**: 10.4 kHz
- **Frequency Accuracy**: Crystal-controlled precision
- **Spurious Emissions**: Compliant with radio regulations

### **Device Specifications**
- **Operating Frequency**: 400-1000 MHz (hardware dependent)
- **Transmission Power**: -9 to +22 dBm (device and firmware dependent)
- **Capcode Range**: 1 to 4,294,967,295 (32-bit addressing)
- **Message Length**: Up to 240 characters (FLEX standard)
- **Binary Data**: Up to 2048 bytes per transmission
- **Serial Interface**: 115200 baud, 8N1 format
- **Power Supply**: 3.3-5V (USB or battery operation)

### **Network Specifications** (v3 Firmware)
- **WiFi Standards**: 802.11 b/g/n (2.4 GHz)
- **Security**: WPA2-PSK encryption
- **IP Assignment**: DHCP client or static IP configuration
- **Web Server**: HTTP on port 80
- **API Server**: HTTP on configurable port (default 16180)
- **Authentication**: HTTP Basic Auth with configurable credentials
- **Concurrent Connections**: Multiple simultaneous web/API clients

### **Performance Characteristics**
- **Boot Time**: <10 seconds to operational state
- **Transmission Latency**: <2 seconds from command to RF output
- **Web Interface Response**: <500ms for typical operations
- **API Response Time**: <200ms for message transmission requests
- **Memory Usage**: Optimized for ESP32 resource constraints
- **Power Consumption**: Optimized for battery operation with sleep modes

---

## 📚 Documentation Ecosystem

### **Getting Started**
- **[QUICKSTART.md](QUICKSTART.md)**: Complete beginner's guide from unboxing to first message transmission
  - Hardware setup and connection procedures
  - Firmware installation with step-by-step instructions
  - First message transmission examples
  - Interface selection guidance for different user types

### **Installation and Setup**
- **[FIRMWARE.md](FIRMWARE.md)**: Comprehensive firmware installation guide
  - Arduino IDE setup and library installation
  - Device-specific flashing procedures with troubleshooting
  - Library dependency management and tinyflex.h embedding
  - Verification procedures and testing protocols

### **User Guides**
- **[USER_GUIDE.md](USER_GUIDE.md)**: Complete web interface user manual
  - WiFi setup and network configuration
  - Web interface navigation and feature explanation
  - Message transmission procedures with examples
  - Configuration management and device customization

### **Technical References**
- **[AT_COMMANDS.md](AT_COMMANDS.md)**: Complete AT command protocol reference
  - Command syntax and parameter specifications
  - Response codes and error handling procedures
  - Usage examples and integration patterns
  - Advanced command sequences and automation

- **[REST_API.md](REST_API.md)**: Comprehensive REST API documentation
  - Endpoint specifications and authentication procedures
  - JSON payload formats and parameter validation
  - Programming examples in multiple languages
  - Integration patterns and best practices

### **Support and Troubleshooting**
- **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)**: Professional troubleshooting guide
  - Hardware detection and connection issues
  - Firmware installation and compilation problems
  - Network connectivity and WiFi configuration
  - GitHub issue reporting procedures with templates

### **Development Documentation**
- **[CLAUDE.md](CLAUDE.md)**: Technical architecture and development notes
  - System architecture and design decisions
  - Development environment setup and procedures
  - Code organization and contribution guidelines
  - Advanced technical implementation details

---

## 🎛️ Use Case Scenarios

### **Amateur Radio Operations**
- **Frequency Experimentation**: Wide frequency range support for band exploration
- **Power Testing**: Variable power output for range and propagation testing
- **Protocol Analysis**: Direct AT command access for technical experimentation
- **Emergency Communications**: Reliable message transmission in emergency scenarios

### **Business and Professional Applications**
- **Staff Notification Systems**: Automated employee paging and alerts
- **Emergency Broadcasts**: Priority message transmission with mail drop support
- **System Integration**: REST API integration with existing business systems
- **Remote Monitoring**: IoT sensor integration with automated alert transmission

### **Home Automation and IoT**
- **Smart Home Integration**: Home Assistant, OpenHAB, and similar platform integration
- **Security System Alerts**: Intrusion detection and alarm transmission
- **Environmental Monitoring**: Sensor-triggered notifications and alerts
- **Family Communication**: Personal paging system for family members

### **Educational and Research**
- **RF Communication Studies**: Hands-on FLEX protocol learning and experimentation
- **Electronics Education**: ESP32 development and radio integration projects
- **Protocol Development**: FLEX standard implementation and analysis
- **System Integration Training**: Multi-interface system design and implementation

### **Legacy System Modernization**
- **Pager System Replacement**: Modern replacement for aging pager infrastructure
- **Protocol Bridge**: Integration between modern systems and legacy pager networks
- **Cost Reduction**: Eliminate recurring pager service fees with self-hosted solution
- **Feature Enhancement**: Add web interface and API capabilities to existing systems

---

## 🔧 Quick Start Summary

### **1. Hardware Acquisition**
Choose your preferred ESP32 LoRa32 development board:
- **TTGO LoRa32-OLED**: Full web interface support, comprehensive documentation
- **Heltec WiFi LoRa 32 V3**: Extended frequency range, professional-grade hardware

### **2. Firmware Installation**
Follow the comprehensive firmware installation guide:
```bash
# See FIRMWARE.md for complete procedures
# Arduino IDE setup, library installation, and flashing
```

### **3. Choose Your Interface**
Select the control method that best fits your needs:
- **Web Interface**: Browser-based control (v3 firmware) → [USER_GUIDE.md](USER_GUIDE.md)
- **AT Commands**: Terminal/serial control → [AT_COMMANDS.md](AT_COMMANDS.md)
- **REST API**: Programmatic control → [REST_API.md](REST_API.md)

### **4. Send Your First Message**
Quick examples for immediate success:
```bash
# Command line (with host application)
flex-fsk-tx 1234567 "Hello World"

# Web interface (v3 firmware)
# http://DEVICE_IP/ → Fill form → Send Message

# REST API (v3 firmware)
curl -X POST http://DEVICE_IP:16180/ -u username:password \
  -H "Content-Type: application/json" \
  -d '{"capcode":1234567,"message":"Hello World"}'
```

**🚀 Complete Beginner?** Start with [QUICKSTART.md](QUICKSTART.md) for step-by-step guidance from unboxing to first transmission.

---

## 🤝 Community and Support

### **Getting Help**
1. **Documentation First**: Check the comprehensive documentation ecosystem above
2. **Troubleshooting Guide**: Review [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common issues
3. **GitHub Issues**: Report problems using the templates in [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
4. **Community Forums**: Engage with other users and developers

### **Contributing**
- **Bug Reports**: Use GitHub issues with detailed templates
- **Feature Requests**: Submit enhancement proposals with use case descriptions
- **Code Contributions**: Pull requests welcome with proper testing
- **Documentation**: Help improve guides and add usage examples
- **Hardware Testing**: Test on additional ESP32 LoRa32 variants

### **Support Channels**
- **GitHub Repository**: Primary support and development coordination
- **Documentation**: Self-service troubleshooting and guidance
- **Community**: User-to-user assistance and experience sharing

---

## 🏆 Acknowledgments

This project builds upon the foundational work of exceptional developers in the amateur radio and open source communities:

### **Core Technology Foundation**
- **[Davidson Francis (Theldus)](https://github.com/Theldus)**: Original tinyflex library architect and FLEX protocol implementation
- **[Rodrigo Laneth](https://github.com/rlaneth)**: Original ESP32 FSK transmitter firmware and hardware integration pioneer
- **tinyflex Project**: [https://github.com/Theldus/tinyflex](https://github.com/Theldus/tinyflex) - Comprehensive FLEX protocol library
- **Original ESP32 Implementation**: [https://github.com/rlaneth/ttgo-fsk-tx/](https://github.com/rlaneth/ttgo-fsk-tx/) - Hardware control foundation

### **Development Ecosystem**
- **Arduino Community**: ESP32 development framework and extensive library ecosystem
- **RadioLib Project**: Advanced radio control library enabling precise FSK transmission
- **ESP32 Community**: Hardware drivers, development tools, and platform support
- **Heltec Automation & LilyGO**: Hardware manufacturers providing excellent development platforms

### **Special Recognition**
The original developers provided not just code, but a vision of accessible, professional-grade paging technology. Their work made it possible to create this standardized, feature-rich system that serves diverse user communities from amateur radio operators to business users.

**This project represents the evolution of their foundational work into a comprehensive, professional-grade solution while maintaining the open source spirit and community-driven development approach.**

---

## 📜 License

This project is released into the **public domain**. This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

---

## 🔗 Project Links

- **Repository**: [https://github.com/geekinsanemx/flex-fsk-tx](https://github.com/geekinsanemx/flex-fsk-tx)
- **Documentation**: Comprehensive guides included in repository
- **Issues**: GitHub issue tracker with professional templates
- **Releases**: Tagged releases with firmware binaries and documentation

---

**Transform your ESP32 LoRa32 device into a professional FLEX paging message transmitter. Join thousands of users worldwide who trust flex-fsk-tx for reliable, feature-rich paging communication.**

📡 **Happy Paging!**