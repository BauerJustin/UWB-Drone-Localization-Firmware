# UWB-Drone-Localization-Firmware
ECE496 Capstone UWB Firmware

ESP32 UWB Indoor Localization code based on https://github.com/jremington/UWB-Indoor-Localization_Arduino 
(which is based on the outdated https://github.com/thotro/arduino-dw1000 library)
For board pin setup and more examples also refer to https://github.com/Makerfabs/Makerfabs-ESP32-UWB

This current implementation does not support more than one tag and up to 4 anchors. 
The boards used are the ESP32 UWB Pro with the Adafruit_SSD1306 OLED display.

For multilateration the program solves the linear least squares problem of computing 
the tag location from known distances and anchor locations. 
For the method, see this short technical paper: https://www.th-luebeck.de/fileadmin/media_cosa/Dateien/Veroeffentlichungen/Sammlung/TR-2-2015-least-sqaures-with-ToA.pdf