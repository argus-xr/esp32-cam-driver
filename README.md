# esp32-cam-driver

This project aims to provide a driver for streaming images from an ESP32-Cam over Wi-Fi for the purpose of SLAM processing on another computer. It is a work in progress. 

The main design objectives are: 
* Sufficient resolution/information for SLAM
* High enough framerate for SLAM
* Low latency

For information on how to set up and program the ESP32-Cam, please refer to https://github.com/argus-xr/argus-xr/wiki/Programming-the-ESP32-Cam.

To compile this project, use Eclipse or VS Code with their respective ESP-IDF plugins, and use ESP-IDF 4.0. Note that the VS Code plugin is kind of dumb and requires your tools folder to be in C:\users\<username>\.espressif, even though it allows you to choose where to put this folder.