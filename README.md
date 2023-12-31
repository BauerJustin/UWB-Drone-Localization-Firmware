# UWB-Drone-Localization-Firmware

ECE496 Capstone UWB Firmware

ESP32 UWB Indoor Localization code based on https://github.com/jremington/UWB-Indoor-Localization_Arduino 
(which is based on the outdated https://github.com/thotro/arduino-dw1000 library)
For board pin setup and more examples also refer to https://github.com/Makerfabs/Makerfabs-ESP32-UWB

This current implementation does not support more than one tag and up to 4 anchors. 
The boards used are the ESP32 UWB Pro with the Adafruit_SSD1306 OLED display.

## Table of Contents

- [Setup](#setup)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshoot] (#troubleshoot)

## Setup

To get started with this project, follow these installtion steps

1. **Clone the Repository:**

    ```bash
    git clone https://github.com/BauerJustin/UWB-Drone-Localization-Firmware.git
    ```

2. **Download Arduino IDE**

    Download from their website at https://www.arduino.cc/en/software

3. **Setup Arduino**

    Setup the necessary board and libraries
    - Tools -> Boards -> Boards Manager -> **install esp32 by Espressif**
    - Tools -> Manage Libraries... -> **Install Adafruit_SSD1306, and ArduinoJson**
    - Move the `DW1000_library` into Arduino's libraries folder (default path is C:\Users\$USER\Documents\Arduino\libraries)
    - Before flashing code, please select **ESP32 wrover** board

## Usage

### setup_anchor

Used for the anchors, make sure to update the MAC address (so anchor 1 = 81, 2 = 82, 3 = 83, 4 = 84)

### tag_main

Main firmware code file for tags.

## Configuration

Here are the pin definitions that should be used for all boards

```c
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

#define UWB_SS 21   // spi select pin
#define UWB_RST 27  // reset pin
#define UWB_IRQ 34  // irq pin

#define I2C_SDA 4	//I2C screen ssd1306
#define I2C_SCL 5
```

WiFi would also need to be configured for the tags in `tag_main`
```c
/****** WIFI CONFIG ********/
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* udpServerIP = "UDP_Server_IP";
const int udpServerPort = 12345;
```
## Troubleshoot

Here are some of the issues I've ran into while working on this project, and their solutions

1. **No COM port for ESP32 device** or **Failed upload**

    Check Device Manager to see if there is a new COM port when you plug in the ESP32. That would be the COM port that would be used to flash the code to ESP32.

    Tools -> Port -> COM#
    
    If no port exists, then you likely don't have its driver. Download it from https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads