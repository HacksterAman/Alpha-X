# Industrial IoT Data Acquisition System

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-blue.svg)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Industrial](https://img.shields.io/badge/Use%20Case-Industrial%20IoT-orange.svg)](https://reflowtech.in)

## Overview

It is a sophisticated dual ESP32-based industrial IoT system designed for high-precision data acquisition and real-time cloud connectivity. The system features two specialized modules working in tandem to provide reliable sensor monitoring, data logging, and remote transmission capabilities for industrial applications.

## System Architecture

The system consists of two main modules running on separate ESP32 microcontrollers:

### 🔧 **M1 Module - Data Acquisition Unit**
- **Primary Function:** High-precision analog data acquisition and local storage
- **Hardware:** ESP32 with dual ADS1115 ADCs and SD card storage
- **Channels:** 6 configurable analog input channels
- **Resolution:** 16-bit ADC resolution with ±4.096V range

### 📡 **M2 Module - Communication Unit**
- **Primary Function:** GSM/4G connectivity and cloud data transmission
- **Hardware:** ESP32 with A7670C GSM module
- **Connectivity:** 4G LTE cellular communication
- **Protocol:** MQTT for real-time data publishing

## Hardware Components

### Core Components
| Component | Quantity | Purpose |
|-----------|----------|---------|
| **ESP32 Microcontrollers** | 2 | Main processing units for each module |
| **ADS1115 16-bit ADCs** | 2 | High-precision analog-to-digital conversion |
| **A7670C GSM/4G Module** | 1 | Cellular connectivity for remote data transmission |
| **SD Card Module** | 1 | Local data storage and backup |
| **EEPROM** | 1 | Persistent parameter storage |
| **Custom PCB** | 1 | Integrates all components with proper connections |

### Input/Output Specifications
- **Analog Inputs:** 6 channels (0.6V - 3.0V range)
- **Digital Communication:** UART, SPI, I2C
- **Storage:** MicroSD card support
- **Power Supply:** 12V DC input with voltage monitoring
- **Communication:** GSM/4G, MQTT over TCP/IP

## Key Features

### 📊 **Data Acquisition (M1)**
- **Multi-channel ADC:** 6 independent analog input channels
- **Configurable Scaling:** Linear mapping with MIN/MAX parameters
- **Mathematical Operations:** Addition, subtraction, multiplication, division
- **Error Detection:** Automatic fault detection for sensor failures
- **Local Storage:** CSV data logging to SD card
- **Parameter Persistence:** EEPROM storage for calibration values
- **Auto-restart:** System stability with 6-hour restart cycles

### 🌐 **Cloud Connectivity (M2)**
- **4G Connectivity:** Automatic APN detection and cellular connection
- **MQTT Protocol:** Real-time data publishing to cloud server
- **Bidirectional Communication:** Remote configuration updates
- **Time Synchronization:** GSM network time for accurate timestamps
- **Data Buffering:** Reliable data transmission with retry mechanisms
- **Remote Monitoring:** Real-time status updates and logging

### 🔧 **Configuration Management**
- **Remote Calibration:** JSON-based parameter updates via MQTT
- **Real-time Updates:** Configuration changes without system restart
- **Parameter Validation:** Precision threshold checking for updates
- **Backup Storage:** EEPROM persistence for critical parameters

## Industrial Use Cases

### 🏭 **Process Monitoring**
- **Chemical Processing:** Real-time monitoring of reactor conditions
- **Temperature Control:** Multi-point temperature monitoring systems
- **Pressure Monitoring:** Critical pressure point surveillance
- **Flow Measurement:** Industrial flow rate monitoring

### 🌡️ **Environmental Monitoring**
- **HVAC Systems:** Building climate control monitoring
- **Water Quality:** Multi-parameter water monitoring stations
- **Air Quality:** Industrial emission monitoring
- **Weather Stations:** Remote meteorological data collection

### ⚡ **Energy Management**
- **Power Monitoring:** Industrial power consumption tracking
- **Battery Systems:** Remote battery bank monitoring
- **Solar Installations:** PV system performance monitoring
- **Generator Monitoring:** Backup power system surveillance

### 🚧 **Asset Monitoring**
- **Equipment Health:** Vibration and condition monitoring
- **Infrastructure:** Bridge and structural health monitoring
- **Remote Assets:** Oil wells, water pumps, and remote machinery
- **Predictive Maintenance:** Early warning system implementation

### 🚜 **Agriculture & Smart Farming**
- **Soil Monitoring:** Multi-point soil condition measurement
- **Irrigation Control:** Automated irrigation system monitoring
- **Greenhouse Management:** Climate control for optimal growing conditions
- **Livestock Monitoring:** Environmental conditions for animal welfare

## Technical Specifications

### Performance
- **Sampling Rate:** Configurable, default 5-second intervals
- **Data Logging:** 1-minute intervals to SD card
- **Cloud Transmission:** 1-minute intervals via MQTT
- **Precision:** 16-bit ADC resolution (0.125mV per bit)
- **Communication Range:** Global 4G LTE coverage

### Power Requirements
- **Input Voltage:** 12V DC
- **Power Consumption:** <2W average operation
- **Backup:** Local SD card storage during connectivity loss
- **Efficiency:** Low-power design for remote installations

### Environmental
- **Operating Temperature:** -20°C to +70°C
- **Humidity:** 0-95% RH (non-condensing)
- **Protection:** Industrial-grade PCB design
- **Mounting:** DIN rail compatible enclosure

## Software Dependencies

### M1 Module Dependencies
```ini
lib_deps = 
    adafruit/Adafruit ADS1X15@^2.5.0
    bblanchon/ArduinoJson@^7.2.0
```

### M2 Module Dependencies
```ini
lib_deps = 
    bblanchon/ArduinoJson@^7.2.0
    vshymanskyy/TinyGSM@^0.12.0
    knolleary/PubSubClient@^2.8
```

## Configuration

### Channel Configuration (JSON Format)
```json
{
  "MIN1": 0.0,
  "MAX1": 100.0,
  "FAC1": 0,
  "CAL1": 0.0,
  "MIN2": 0.0,
  "MAX2": 50.0,
  "FAC2": 2,
  "CAL2": 1.5
}
```

### Factor Operations
- **0:** Addition (output = scaled_value + calibration)
- **1:** Subtraction (output = scaled_value - calibration)
- **2:** Multiplication (output = scaled_value × calibration)
- **3:** Division (output = scaled_value ÷ calibration)

## Data Format

### Output Data Structure
```json
{
  "RawCH1": "1.25",
  "CH1": "25.50",
  "ERR1": 0,
  "RawCH2": "2.10",
  "CH2": "42.75",
  "ERR2": 0,
  "UpdateTimeStamp": "15/12/23 14:30:25"
}
```

### Log Data Structure
```json
{
  "DeviceId": "AX301",
  "Timestamp": "2023-12-15 14:30:00",
  "CH1": "25.50",
  "CH2": "42.75",
  "CH3": "18.20"
}
```

## Installation & Setup

### Prerequisites
- PlatformIO IDE or Arduino IDE with ESP32 support
- Active 4G SIM card with data plan
- MicroSD card (Class 10 recommended)

### Hardware Setup
1. Mount the custom PCB in appropriate enclosure
2. Connect analog sensors to designated input channels
3. Insert configured SIM card into GSM module
4. Connect 12V DC power supply
5. Insert formatted microSD card

### Software Configuration
1. Update MQTT credentials in M2 code
2. Configure APN settings if required (auto-detection enabled)
3. Set device ID and channel parameters
4. Flash firmware to both ESP32 modules

## Cloud Integration

### MQTT Topics
- **Input:** `AX3/01/INPUT` (Configuration updates)
- **Output:** `AX3/01/OUTPUT` (Real-time data)
- **Log:** `AX3/01/LOG` (Historical logging)

## Troubleshooting

### Common Issues
1. **ADC Initialization Failure:** Check I2C connections and addresses
2. **GSM Connection Issues:** Verify SIM card and network coverage
3. **SD Card Errors:** Ensure proper formatting and card compatibility
4. **MQTT Disconnections:** Check network stability and credentials

### Diagnostic Features
- Serial debugging output at 115200 baud
- LED status indicators for system health
- Automatic error reporting via MQTT
- Local data backup during connectivity loss

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
