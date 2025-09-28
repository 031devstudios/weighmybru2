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

Installation instructions are currently under development, for now follow the [link](https://031devstudios.github.io/weighmybru-docs/#/installation/flashing) for step-by-step installation instructions.

```
  this project requires VSCode with PlatformIO extension installed
```

## Bill Of Materials (BOM)

| Qty |           Item                      | 
| --- | ----------------------------------- |  
|  1  | 500g Mini Loadcell (I-shaped)       |  
|  1  | HX711                               |
|  1  | ESP32-S3-Supermini Board            |
|  2  | Capacitive Tocuh Pads               |
|  1  | 0.91" SSD1306 OLED Display          |
|  1  | 800mAh Li-ion Battery               |
|  1  | JST-PH 2.0 Male Connector           |
|  1  | Slide Switch                        |
|  1  | Various Wires                       |
|  2  | M3x5x4 Heat Set Inserts             |
|  16 | M1.7x4 Self Tapping Screws          |
|  4  | M3x12 Button Head Screws            |
|  1  | Female 2 Pin JST Connector(battery) |
|  2  | 100K ohm 1% 1/4w resistors          |
|  4  | Self-Adhesive Rubber Feet           |
|  1  | Double Sided Tape (To hold Battery) |



##           Printed Parts (Found in CAD Folder)

| Qty |           Item                    | 
| --- | ----------------------------------|  
|  1  | Bottom*                           |  
|  1  | Top                               |
|  1  | ESP32 Clamp                       |
|  1  | Screen Clamp                      |
|  4  | washers (Used for HX711)          |

\* Bottom has 2 options - supported has engineered supports while the standard requires you to slice with your own supports

