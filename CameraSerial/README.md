# CameraSerial
This program takes a photo with the ESP32-CAM and prints it as a BMP hex dump to serial.  It is intended as a test of the ESP32-CAM without a dependency on Wi-Fi configuration and networking.  Instructions for uploading and usage are below.  

### Uploading
To upload and use this program, you can use the Arduino IDE or another environment capable of uploading to the ESP32-CAM and communicating with it over serial.  These instructions assume you have set up the Arduino IDE for uploading to the ESP32-CAM; for details, see [Programming the ESP32 Cam](https://github.com/argus-xr/argus-xr/wiki/Programming-the-ESP32-Cam) on the `argus-xr` wiki. 

If you haven't done so already, clone this repository with `git clone https://github.com/argus-xr/esp32-cam-driver`. In the Arduino IDE, go to File > Open... and navigate to `CameraSerial.ino` in the `esp32-cam-driver/CameraSerial` folder. To put the ESP32-CAM in upload mode, attach a wire connected to ground to the `IO0` pin (three pins to the left of the `VnR` and `VOT` pins used for I2C serial communication). Then, either disconnnect and reconnect the wire to the `5V` pin (should be easy with a breadboard setup), or alternatively, press and release the reset button located on the back side of the board (opposite the camera side, where the header pins stick out) next to the `5V` pin.  If you use the reset button, you should see the flash LED light up while the button is pressed.  If the serial monitor is connected, it should print `waiting for download`.  The ESP32-CAM is now ready for uploading. 

In the Arduino IDE, select the Upload button (arrow pointing to the right).  In the console at the bottom, you should see something like `Writing at 0x00010000... (10 %)`.  Once it finishes uploading, it prints `Hard resetting via RTS pin...`.  Remove the ground wire from the `IO0` pin.  Open the Serial Monitor in the Arduini IDE (located at Tools > Serial Monitor), then press the reset button on the ESP32-CAM.

### Usage
Once you press the reset button, the program will start. It should print `Taking picture in 5...`. If there is an error, it prints "Camera init failed with error" followed by the error code. (This may be a sign that the board model/pin layout selection is incorrect.. It counts down for five seconds, then prints `*click*` with a newline as it takes the picture. It prints `buf\_len: ` followed by the number of bytes, then dumps a BMP image in hex format to the terminal. All in all, it should look something like this:

```
Taking picture in 5... 4... 3... 2... 1... *click*

buf_len: 57654
424D36E10000000000003600000028000000A000000088FFFFFF010018000000
[ many lines of hex data omitted ]
80909880889880889880809878809480889480889480
```

Select the lines of hex data and copy them to clipboard. Open a terminal window, navigate to the CameraSerial folder, and run `./hex_to_file.py`. It prompts you for a `Filename (ending in .bmp):`. Once you enter a filename, follow the instructions it prints: `Paste hexdump, then press Enter twice.. You should now have a .bmp file with the filename you entered. Open the file with an image viewer of your choice. If all goes well, you should see a photo from the ESP32-CAM. 
