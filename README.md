# Darkroom Exposure Meter

> [Инструкция на русском](./README_RU.md)

[3D Model](https://www.thingiverse.com/thing:7319866)

[YouTube playlist with description, demonstration, and assembly](https://youtube.com/playlist?list=PLLB8YXu4D8PunXm6wkRArw5l53CUL2j-Z&si=r77Fl3qdb0qFjc_D)

## Description

This is an densitometer for the darkroom. It allows you to measure negative contrast and obtain estimated printing times.

The device is designed to be extremely simple. The interface consists of just one knob and 2 buttons (one integrated into the encoder).

Common components are used for the assembly. The only specialized component is OPT101 photodiode.

### Controls

1. Upon startup, the device shows the amount of incident light in logD. `30 logD == 1 stop`. The higher the value, the "darker" the incident light. The values are relative. For example, a value of 50 relative to 90 means `the light is brighter by 40 logD (4/3 stops)`
2. Press the side button to start a measurement relative to the current value (`-` means the light is brighter, `r` -- darker)
3. Hold the side button to return to absolute values
4. Press the encoder to show the approximate printing time calculated from the base time. Press the encoder again to return to absolute values
5. Hold the encoder to enter the base time setting menu (see details below). Pressing the encoder selects the value to be set in this menu. Hold the encoder to get back
6. Hold both the encoder and the measurement button to turn off the device. Also, It turns off automatically after 2 minutes

### Common measurement tips

1. Work with dimmed or turned off red light
2. Remove your phone from darkroom. It could emit IR light (LiDAR, FaceID)
3. Control angle of measured light. Use plexiglass scope for it

### Calibration

It is recommended to calibrate the device before use. To do this:
1. Enter a darkroom with no light at all
2. Additionally cover the sensor with something opaque
3. Hold the encoder and measurement button for more than 5 seconds. The device starts calibration
4.` The screen will be turned off during calibration. Calibration takes approximately 1 minute

### Setting the Base Time

1. Print an ideally exposed print
2. Measure the absolute logD values of some segment of the printed negative. Not change the enlarger parameters: height, aperture. You can measure highlights, shadows, skin tones, etc.
3. Record the measured values in a notebook in the following format:

```
Object description on negative | logD | Printing time
```

Example:

```
skin in shadow    |    323     |   0:40
bright highlights |    220     |   0:40
```

In the future, you can also add paper names, grade and other parameters to this table.

4. Switch to base time setting mode
5. Set the base logD
6. Press the encoder. Set the minutes
7. Press the encoder. Set the seconds
8. Hold the encoder to save the value and return to absolute logD mode

Now, when you press the encoder, the device will show you the estimated printing time relative to the specified base logD and time.

Example:

1. You set a base value of `logD = 200, Time = 00:36` for skin in shadow
2. You set up another negative that also has skin in shadow
3. You measure this skin, and the device shows you `logD == 250`
4. By pressing the encoder, the device will show you an estimated printing time `01:54` (`114 seconds == 36 * 2^(5/3)`)

### How to Measure Negative Contrast

1. Find the darkest or brightest area on the negative in absolute values
2. Press the side button to start calculating values relative to it
3. Now find the opposite (brightest/darkest) area
4. The resulting value is the negative contrast in logD

Tips:

1. Raise the enlarger to the very top to make it easier to measure small objects
2. Open the aperture on the enlarger

## Assembly

### Required Components

1. **Arduino Nano Type-C.** You will need to remove the power LED. So, it doesn't glow inside the case. Also, to reduce power consumption, snip off the left leg of the voltage regulator. [More details here](https://alexgyver.ru/lessons/power-sleep/#2-toc-title). The corners of the board will need to be trimmed to fit into the case
2. **Tactile button 12x12x9mm.** The height can vary—you just need to choose a matching cap
3. **Encoder EC11.** You will need to sawn off the some part of top of it to fit the knob
4. **4-segment display** on a TM1637 driver
5. **Analog multiplexer 74HC4067.** Used for switching the gain resistors on the photodiode
6. **16-bit ADC ADS1115.** Used for reading values from the photodiode
7. **Photodiode module OPT101.** Get the version with mounting PCB. Sold on AliExpress. [Verified Link 1](https://ali.click/hw0l314), [Verified Link 2](https://ali.click/wy0l31o)
8. **100kOhm resistor**
9. **3 resistors of 10mOhm or 1 of 30mOhm.** I connected three 10mOhm resistors to get a 30mOhm resistor
10. **Copper tape** for shielding
11. **Wires**
12. **M3 nuts and screws** (5mm and 10mm, or slightly longer)
13. **2mm Plexiglass** to make the sight for aiming at the photodiode

The following components are optional, in case you want to make the device autonomous (battery-powered):

1. **TP4056 charging board**
2. **Voltage converter CKCS BS01 (5V)**
3. **Battery** up to 10x34x50mm. I used a Li-Po 103450 2000 mAh 3.7V
4. **Blue plastic light filter 1.5mm.** Green can also be used. Its filter part of the ambient red light
5. **Translucent acrylic.** Used for style and screen dimming

### Circuit

> *Note:* The dotted lines mark the battery power section. If you don't need the device to be autonomous, you can omit it.

![Circuit](./docs/Circuit.jpg)

### Assembly Notes

1. The photodiode is very sensitive to external interference. Therefore, You will need to shield device case with copper tape. Ground should be connected to it.
2. Do not overheat the photodiode during soldering. It can be easily damaged. Give it some time to cool down.
3. Align the diode in the center of the hole.
4. The battery is located under the photodiode. To prevent the diode's pins from shorting against it, isolate the battery with electrical tape.
5. After assembling the device, ensure that no components are emitting light. You can break off any glowing LEDs.
6. Check the current draw from the battery. It should be around **0.3mA** when the device is in sleep mode.

### Firmware

The easiest way to upload the firmware is via the Arduino IDE:

1. Install the Arduino IDE
2. Clone this git project into any directory
3. Open the .ino file via the Arduino IDE (`File` -\> `Open...` -\> `path to .ino file`)
4. Install the project dependencies (`Tools` -\> `Manage Libraries...`):
      * EncButton (by Alex Gyver)
      * GyverIO (by Alex Gyver)
      * GyverSegment (by Alex Gyver)
      * GyverPower (by Alex Gyver)
      * PinChangeInterrupt (by Nico Hood)
      * CRC32 (by Christopher Baker)
      * ADS1X15 (by Rob Tillaart)
5. Select board — Arduino Nano and processor — ATmega328P
6. Click the Upload button

## Support

tg: @lo1ol
email: myprettycapybara@gmail.com
