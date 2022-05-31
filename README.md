
![teaser_4](https://user-images.githubusercontent.com/15098003/171178685-e226b7ec-bc1c-44df-a134-f5b71171801a.gif)


<h1 align="center">
    E-TKT: anachronic label maker
</h1>


<p align="center">Andrei Speridião 2022 - http://andrei.cc</p>


### table of contents

- [what is it?](#-what-is-it)
- [why?](#-why)
- [how does it work?](#%EF%B8%8F-how-does-it-work)
- [features](#-features)
- [components](#-components)
- [3D printing](#-3d-printing)
- [to do](#%EF%B8%8F-to-do)
- [libraries](#-libraries)
- [license](#%EF%B8%8F-license)

# 🙃 what is it?

> ### *étiquette f (plural étiquettes)*
> ["ticket, memorandum, attach, stick, pierce, sting, to be sharp, goad, puncture, attach, nail"](https://en.wiktionary.org/wiki/%C3%A9tiquette#French)
> 1. tag, label 
> 2. prescribed behavior

E-TKT is a DIY label maker that mixes both old fashioned and contemporary technology to create something as simple as... Labels!

[![🎥 see it working 🎥](https://user-images.githubusercontent.com/15098003/171185500-8a63297c-487c-4900-b6d2-5c67298541d4.png)](https://www.youtube.com/watch?v=5hv-2kIJUVc "🎥 see it working 🎥")



# 🤔 why?

### *TLDR: curiosity, technical challenge and of course organizing stuff.*

<p align="center">
    <img src="https://user-images.githubusercontent.com/15098003/171187101-0a5633a8-f852-488e-8685-b5fae7079eef.png" width="40%">    
</p>

The initial spark for this project came from an ordinary handheld [labeling device](https://www.aliexpress.com/item/3256801648218535.html) that I bought to organize my workshop. I was ***VERY UPSET*** when I noticed that it was basically rubbish, getting broken and unusable after just a few operations.



I knew I could easily buy a common electronic thermal label printer, but something about the vintage embossed finish fascinated me. Furthermore, even though the pressing mechanism was poorly made, the characters' carousel was pretty sturdy and sharp. So I thought: can I build over its principles to create a functional device? If so, then why not make it physical-digital?

Throughout the process I have constantly questioned what made me develop this ambiguous device, with no clear answer.

Finally, when creating the project video I've realized that "anachronism" is what I’ve been attracted to. According to [Wikipedia](https://en.wikipedia.org/wiki/Anachronism):
> “a chronological inconsistency in some arrangements, especially a juxtaposition of people, events, objects, language terms and customs from different time periods”.

### The point is: even though the process is digitalized, the resulting label is totally old school and there is no easy way of telling if it wasn't made in the traditional way.

...why bother mixing an archaic printing method with current automation and connectivity features? Because why not? It was a technological blind spot to be explored!



# ⚙️ how does it work?
![how](https://user-images.githubusercontent.com/15098003/171194737-37861a1f-fba7-404c-b987-5b3d26e704f3.png)

An ESP32 commands the label production and also serves an on demand web application to any device connected in a local network. Neither Internet is needed, nor installing any app.

The whole process of connecting the E-TKT machine to a local network and then launching the app is aided by a small OLED screen that provides instructions and a dynamically generated QR code with the URL, according to the IP attributed by the WLAN.

The web app provides text validation, special characters, a preview of the exact size of the tape, an option to select the desired lateral margins and also real-time feedback during the printing (also present on the device screen). There are also commands for attaching a new reel, manually feeding and cutting the tape.

The label production itself uses the same mechanical principles as the original machine did, but is now automated. A stepper motor feeds the tape while another selects the appropriate character on the carousel according to a home position acquired by hall sensor. Then a servo motor imprints each character by pressing the carousel to the tape. That happens successively until the end of the desired content, when there is a special character position to cut the label. A light blinks to ask for the label to be picked.


# 💡 features
## 🌟 *highlights*
- No need for internet, app installation, data cables or drivers;
- Use from any device that is connected to a local network and has a web browser: desktop, tablet, smartphone;
- Compatible with cheap 9mm generic DYMO-compatible tape;

## 📱 web app

<img src="https://user-images.githubusercontent.com/15098003/171068984-5492a5b2-0eec-4714-9bf8-0055f606ac5b.gif" height="600">

- Instant preview: what you see is what you’ll get;
- Real time check for character validity;
- Label length estimation;
- Margin modes: tight, small (1 space each side) or full (max input length);
- Buttons for special characters: ♡  ☆  ♪  $ @
- Reeling function: for when a new tape reel must be installed;
- Manual commands: feed & cut;

## 🔌 device
![8522_t](https://user-images.githubusercontent.com/15098003/171065272-df92a233-937b-404c-a1b7-b58c65ff6560.jpg)
![8574_t](https://user-images.githubusercontent.com/15098003/171067234-45f603e2-b86b-484a-a918-976d7dfe89cd.jpg)<img src="https://user-images.githubusercontent.com/15098003/171069105-5f6ff133-97fa-4558-84bd-1174a9965873.jpg" width="50%"><img src="https://user-images.githubusercontent.com/15098003/171069109-fd11ad4a-c32e-40f8-b43b-75f63577aefb.jpg" width="50%"><img src="https://user-images.githubusercontent.com/15098003/171069110-bae5d936-b745-4c0f-98dc-123e3d544d0c.jpg" width="50%"><img src="https://user-images.githubusercontent.com/15098003/171069111-e2f2c641-3e2e-4332-af1f-f96e201be519.jpg" width="50%">
![8501_t](https://user-images.githubusercontent.com/15098003/171069838-1836ead2-5ab6-490b-a2e7-acff03536e2e.jpg)



- Minimum label size to allow for picking it up;
- OLED screen + LED feedback:
- Instructions for configuring the wifi;
- QR code/URL for easily accessing the web app;
- Real time progress;


# 🧩 components

The estimated cost is around $70 (USD) without shipping, as of May 2022.
 
| ID | TYPE | SUBSYSTEM | PART - DESCRIPTION | QTY | REF | 
| :---: | :---: | :---: |  :--- | :---: | :---: |
| 1 | structure | - | 3D print filament - PETG (~200g used) | 0.2 | [link](http://prusa3d.com/product/prusament-petg-jet-black-1kg/) |
| 2 | structure | - | screw - M3x40mm | 1 | [link](http://aliexpress.com/item/2261799963738734.html) |
| 3 | structure | - | screw - M3x20mm | 17 | [link](http://aliexpress.com/item/2261799963738734.html) |
| 4 | structure | - | screw - M3x18mm | 3 | [link](http://aliexpress.com/item/2261799963738734.html) |
| 5 | structure | - | screw - M3x16mm | 2 | [link](http://aliexpress.com/item/2261799963738734.html) |
| 6 | structure | - | screw - M3x8mm | 6 | [link](http://aliexpress.com/item/2261799963738734.html) |
| 7 | structure | - | screw - M3x6mm | 10 | [link](http://aliexpress.com/item/2261799963738734.html) |
| 8 | structure | - | screw - M4x12mm | 1 | [link](http://aliexpress.com/item/2261799963738734.html) |
| 9 | structure | - | hex nut - M3 | 10 | [link](http://aliexpress.com/item/1005001966426139.html) |
| 10 | structure | - | washer - M3 | 9 | [link](http://aliexpress.com/item/3256801295230574.html) |
| 11 | structure | - | silicone pad feet - 8x2mm | 4 | [link](http://aliexpress.com/item/2251832637568894.html) |
| 12 | mechanic | extruder | spring - 7mm ⌀ x 6mm length | 1 | [link](http://aliexpress.com/item/4001179419287.html) |
| 13 | mechanic | carousel | pressing carousel - MOTEX / CIDY | 1 | [link](http://aliexpress.com/item/3256803798622137.html) |
| 14 | mechanic | carousel | bearing - 608ZZ | 1 | [link](http://aliexpress.com/item/1005001813219171.html) |
| 15 | mechanic | extruder | bearing - 623ZZ | 2 | [link](http://aliexpress.com/item/1005001813219171.html) |
| 16 | mechanic | extruder | extruder gear - MK8 40 teeth / 5mm axis | 1 | [link](http://aliexpress.com/item/2255800252771556.html) |
| 17 | electronic | - | dual side prototyping PCB - 0.1" pitch - 50x70x1.6mm | 1 | [link](http://aliexpress.com/item/4000062405721.html) |
| 18 | electronic | carousel | hall sensor - KY-003 | 1 | [link](http://aliexpress.com/item/2251832475321023.html) |
| 19 | electronic | carousel | stepper motor - NEMA 17HS4023 | 1 | [link](http://aliexpress.com/item/2251832620474591.html) |
| 20 | electronic | carousel | stepper driver - A4988 | 1 | [link](http://aliexpress.com/item/3256801435362018.html) |
| 21 | electronic | extruder | stepper motor (28BYJ-48 5V) & driver (ULN2003) | 1 | [link](http://aliexpress.com/item/1005003353402464.html) |
| 22 | electronic | press | servo - Towerpro MG-996R | 1 | [link](http://aliexpress.com/item/2251832857187114.html) |
| 23 | electronic | carousel | neodymium magnet - 2mm ⌀ x 3mm length | 1 | [link](http://aliexpress.com/item/3256803632497346.html) |
| 24 | electronic | - | ESP32 WROOM nodeMCU | 1 | [link](http://aliexpress.com/item/2251832741952874.html) |
| 25 | electronic | level | 4-channel I2C-safe Bi-directional Logic Level Converter - BSS138 | 1 | [link](http://adafruit.com/product/757) |
| 26 | electronic | UI | OLED screen - 128x32px 0.91" | 1 | [link](http://aliexpress.com/item/32927682460.html) |
| 27 | electronic | UI | LED - white PTH 2x5x7mm square | 2 | [link](http://aliexpress.com/item/3256803160975747.html) |
| 28 | electronic | power | step down - 6V - LM7806 | 1 | [link](http://aliexpress.com/item/32965210867.html) |
| 29 | electronic | power | power supply - 7V 5A | 1 | link |
| 30 | electronic | power | DC-005 Power Jack Socket | 1 | [link](http://aliexpress.com/item/2251801542561009.html) |
| 31 | electronic | wifi reset | tact switch - 6x6x4.5 | 1 | [link](http://aliexpress.com/item/3256802537583003.html) |
| 32 | electronic | - | wiring - solid (prototype) | 1 | link |
| 33 | harness | - | USB type-A to micro-B data cable | 1 | [link](http://aliexpress.com/item/2255800229926282.html) |
| 34 | harness | - | wiring (harnesses) - flexible, flat | 1 | [link](http://aliexpress.com/item/2251832639497810.html) |
| 35 | harness | - | header jumper connector | 1 | [link](http://aliexpress.com/item/2251801839907761.html) |
| 36 | harness | - | male header - 0.1" pitch | 2 | [link](http://aliexpress.com/item/2251832538163556.html) |
| 37 | harness | - | female header - 0.1" pitch | 2 | [link](http://aliexpress.com/item/2251832538163556.html) |
| 38 | harness | - | female connector - 8 pin 0.1" pitch dupont | 1 | [link](http://aliexpress.com/item/3256802073547679.html) |
| 39 | harness | - | female connector - 3 pin 0.1" pitch dupont | 4 | [link](http://aliexpress.com/item/3256802073547679.html) |
| 40 | harness | - | female connector - 4 pin 0.1" pitch dupont | 2 | [link](http://aliexpress.com/item/3256802073547679.html) |
| 41 | consumable | - | label tape - 9mm DYMO compatible | 1 | [link](http://aliexpress.com/item/1005001525284316.html) |


# ⚡ electronics

![Schematic_e-tkt_2022-05-30](https://user-images.githubusercontent.com/15098003/171064999-262a4c68-01ae-4122-8584-5d784ebf6408.png)

### Power
  - *7-12v* to be provided by an external power supply with at least 35w to deal with servo peaks current while pressing the label. It supplies the stepper drivers directly.
  - *6v* out of the L7806 step down and is provided for both the servo and hall sensor.
  - *3.3v* is provided by the ESP32 WROOM board (as in its logical ports).

### Logic Level
- as the ESP32 uses 3.3v logic, we need this conversion for parts that are running on higher voltage (servo and hall sensor).

### Press
- the servo uses 6v, higher voltages tend to damage it.

### Carousel
- NEMA stepper, driver and a hall sensor to match the position origin.

### Wifi reset
- a tact button that when pressed while booting, clears the saved credentials.

### Feeder
- reduced stepper motor along with its standard driver.

### User interface
- running on 3.3v an I²C OLED display and two LEDs (no need for resistors).



# 🧵 3D printing
![exploded](https://user-images.githubusercontent.com/15098003/171068151-33b3fd52-b4f0-49f8-ad5a-521146b65bbb.png)

***<details><summary> 📃 16 parts in total, using approx 200g of PETG filament.</summary>***
<p>
       
###  List of printed parts:
- A_bottom
- B_wall
- C_wall_track
- D_pillar_1
- E_pillar_2
- F_pillar_3
- G_pillar_4
- H_pillar_5
- I_top
- J_top_screenholder
- K_top_tapefeed
- L_caroulsel_cube
- M_carousel_hallholder
- N_carousel_coupling_1
- O_carousel_coupling_2
- P_press
    
</p>
</details>


### Settings
- layer height: 0.25mm
- infill: 20%
- wall line count: 3 + 1 (Cura usually adds the later together with the infill)
- top/bottom layers: 4

*PS: some parts might need support.*



# 🛠️ to do
- [ ] [Bugfix](https://github.com/andreisperid/E-TKT/issues?q=is%3Aopen+is%3Aissue+label%3Abug)
- [ ] Manufactured PCB

***<details><summary> A bit futher into the process 🌑🌘🌗🌖🌕 </summary>***
<p>

### I - Experiment (March 2021)
- [x] Carousel homing with infrared sensor + one missing “teeth” led erratic results
- [x] Testing with SG-90 servo, but it was too weak to press the tape
- [x] Using Arduino mega, communication via serial

### II - Communication
- [x] Experiments using ESP8266 with self served app + receiving commands, still isolated from the main functionality
- [x] Wifi manager
- [x] First sketches for the web app user interface

###  III - Printing Proof
- [x] Dual core tasks: one serves the network/app and the other controls all physical operations
- [x] Stronger MG-996R servo resulted on successful tag printing
- [x] Hall sensor for homing with precision
- [x] Carousel direction on clockwise only to avoid tape screwing
- [x] Migration for the ESP32

###  IV - Consistency and Usability
- [x] More compact device, with smaller stepper motor for the carousel
- [x] Tested using NFC to open web app on smartphone, failed (security standards)
- [x] OLED display to help on configuration and feedback
- [x] QR code as an alternative to quickly open web app
- [x] Improvements on web app usability and visuals

### V - Optimization, Extras and Documentation (May 2022)
- [x] Implemented special symbols
- [x] Few printing optimizations
- [x] Documentation with texts, schemes, photos and video
    
</p>
</details>



# 📚 libraries

Framework: arduino

Environment: nodemcu-32s @3.3.2

- [waspinator/AccelStepper@1.61](https://github.com/waspinator/AccelStepper )

- [madhephaestus/ESP32Servo@0.9.0](https://github.com/madhephaestus/ESP32Servo )

- [ottowinter/ESPAsyncWebServer-esphome@1.2.7](https://github.com/me-no-dev/ESPAsyncWebServer)

- [alanswx/ESPAsyncWiFiManager@0.24](https://github.com/alanswx/ESPAsyncWiFiManager)

- [olikraus/U8g2@2.28.8](https://github.com/olikraus/U8g2)

- [ricmoo/QRCode@0.0.1](https://github.com/ricmoo/QRCode) 



# ⚖️ license

MIT @ [Andrei Speridião](https://github.com/andreisperid/)

If you ever build one, I would love to know ;)

## ...and you are more than welcome to visit [andrei.cc](https://andrei.cc) !
