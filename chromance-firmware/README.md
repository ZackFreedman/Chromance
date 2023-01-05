How to Setup
---
- Open up [chromance-firmware.ino](https://github.com/ZackFreedman/Chromance/blob/main/chromance-firmware/chromance-firmware.ino)
- Edit the following
  - `ssid`: With your WIFI SSID
  - `password`: With your WIFI Password
  - Set your LED Type to `#define __LED_TYPE NEOPIXEL` for NEOPIXEL or `#define __LED_TYPE DOTSTAR` for DOTSTAR LEDS
  - 

Optionally Edit the Following as needed:
  - Comment out `#define USING_EMOTI_BIT_SENSOR` if you aren't using an emoti bit sensor
  - `blueStripDataPin`: With the Data Pin you are using for the blue
  - `greenStripDataPin`: With the Data Pin you are using for the green
  - `redStripDataPin`: With the Data Pin you are using for the red
  - `blackStripDataPin`: With the Data Pin you are using for the black
  - `blueStripClockPin`: With the Clock Pin you are using for the blue
  - `redStripClockPin`: With the Clock Pin you are using for the red
  - `greenStripClockPin`: With the Clock Pin you are using for the green
  - `blackStripClockPin`: With the Clock Pin you are using for the black