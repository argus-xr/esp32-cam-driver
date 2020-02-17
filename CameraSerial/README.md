# CameraSerial

### Uploading


### Usage
Once you have uploaded the program, reset the ESP32-CAM with the serial monitor connected. It should print `Taking picture in 5...`.  If there is an error, it prints "Camera init failed with error" followed by the error code.  (This may be a sign that the board model/pin layout selection is incorrect.)  It counts down for five seconds, then prints `*click*` with a newline as it takes the picture.  It prints `buf_len: ` followed by the number of bytes, then dumps a BMP image in hex format to the terminal. All in all, it should look something like this:

```
Taking picture in 5... 4... 3... 2... 1... *click*

buf_len: 57654
424D36E10000000000003600000028000000A000000088FFFFFF010018000000
[ many lines of hex data omitted ]
80909880889880889880809878809480889480889480
```

Select the lines of hex data and copy them to clipboard.  Open a terminal window, navigate to the CameraSerial folder, and run `./hex_to_file.py`.  It prompts you for a `Filename (ending in .bmp):`.  Once you enter a filename, follow the instructions it prints: `Paste hexdump, then press Enter twice.`  You should now have a .bmp file with the filename you entered.  Open the file with an image viewer of your choice.  If all goes well, you should see a photo from the ESP32-CAM.  
