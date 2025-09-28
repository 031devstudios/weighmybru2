<p align="center">
<img src="https://raw.githubusercontent.com/031devstudios/weighmybru/refs/heads/master/docs/assets/logo.png" alt="WeighMyBru Dashboard" width="500" height="745"/>
</p>

<p align="center">  <b>The smart coffee scale that doesn't break the bank!</b></p>
<br>

[![](https://dcbadge.limes.pink/api/server/HYp4TSEjSf)](https://discord.gg/HYp4TSEjSf)
[![License](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg?style=for-the-badge)](LICENSE)

This project is a smart coffee scale with a webserver hosted on the ESP32-S3.
This scale was designed to be used in conjunction with GaggiMate, but can also be used as a standalone scale. The project is still in a beta phase and is actively being worked on. Please head over to the [Discord](https://discord.gg/HYp4TSEjSf) server for more info. This scale was inspired by [EspressiScale](https://www.espressiscale.com) but with a non-custom PCB approach. The idea is to be low-cost, easily sourcable scale.

<br>
<br>
<p>
<img src="docs/assets/dashboard.jpg" alt="WeighMyBru Dashboard" width="250" />
<img src="docs/assets/weighmybru.jpg" alt="WeighMyBru Dashboard" width="700" />
</p>
## Documentation

The documentation is currently being worked on. 

[WeighMyBru Docs](https://031devstudios.github.io/weighmybru-docs/)

## Features

- Webserver via WiFi
- Bluetooth connectivity to GaggiMate
- Calibration via webserver
- Real-time flowrate display
- Adjustable decimal point readings
- Different modes for to cater for espresso and pour-overs

## GaggiMate

GaggiMate now fully supports WeighMyBru scale.

[GaggiMate](https://github.com/jniebuhr/gaggimate)

## Installation

Installation instructions are currently under development, for now follow the [link](https://031devstudios.github.io/weighmybru-docs/#/installation/flashing) for step-by-step installation instructions. Additionally, a video is available on [youtube](https://www.youtube.com/watch?v=O5SP40Liuq0)

```
  this project requires VSCode with PlatformIO extension installed
```

## Bill Of Materials (BOM)

| Qty |           Item                      | Amazon Link | Aliexpress Link |
| --- | ----------------------------------- | -----------------------| --------------- |
|  1  | 500g Mini Loadcell (I-shaped)       | https://a.co/d/6kvxZ0H |  |
|  1  | HX711                               | https://a.co/d/3KiYRkA |  |
|  1  | ESP32-S3-Supermini Board            | https://a.co/d/6289vbS |  |
|  2  | Capacitive Tocuh Pads               | https://a.co/d/014di6l |  |
|  1  | 0.91" SSD1306 OLED Display          | https://a.co/d/9UClWku |  |
|  1  | 800mAh Li-ion Battery               | https://a.co/d/gbr1Yft |  |
|  1  | JST-PH 2.0 Male Connector           | https://a.co/d/3BZGuHW |  |
|  1  | Slide Switch                        | https://a.co/d/9KRqMyF |  |
|  1  | Hookup Wire (Various Colors)        | https://a.co/d/1Fs8os9 |  |
|  2  | M3x5x4 Heat Set Inserts             | https://a.co/d/bnQD7Iu |  |
|  16 | M1.7x4 Self Tapping Screws          | https://a.co/d/1np5Nes |  |
|  4  | M3x12 Button Head Screws            | https://a.co/d/iqM3d6E |  |
|  2  | 100K ohm 1% 1/4w resistors          | https://a.co/d/3R0YmGM |  |
|  4  | Self-Adhesive Rubber Feet           | https://a.co/d/0q9TRmR |  |
|  1  | Double Sided Tape (To hold Battery) | https://a.co/d/gM5SWwH |  |



##           Printed Parts (Found in CAD Folder)

| Qty |           Item                    | 
| --- | ----------------------------------|  
|  1  | Bottom*                           |  
|  1  | Top                               |
|  1  | ESP32 Clamp                       |
|  1  | Screen Clamp                      |
|  4  | washers (Used for HX711)          |

\* Bottom has 2 options - The supported option has engineered supports, while the standard option requires you to slice with your own supports.

