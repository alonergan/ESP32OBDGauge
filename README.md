# ESP32 OBD Gauge

ESP32 OBD Gauge is a touch-controlled, 320×240 automotive display built around an ESP32, a BLE ELM327-compatible OBD-II adapter, and an MPU6050 accelerometer. It provides configurable single, dual, quadrant, G-force, and 0–60 mph displays with persistent per-screen themes and gauge assignments.

> **Safety:** Configure and test the display while the vehicle is parked. Do not operate menus or troubleshoot hardware while driving. OBD-II PID availability varies by vehicle; unsupported values should not be treated as authoritative vehicle diagnostics.

## Capabilities

- Five display modes: single needle gauge, dual bar gauge, G-meter, acceleration meter, and four-quadrant gauge.
- BLE discovery, pairing, reconnecting, storage, and removal for compatible OBD adapters.
- Up to four saved BLE devices.
- Configurable OBD reading for the single gauge, both sides of the dual gauge, and every quadrant.
- Double-tap shortcuts for changing gauge assignments.
- Independent value, label, and outline colors for each gauge screen.
- Detailed HSV-style palette, reusable favorite colors, and a predefined swatch grid.
- Swatches organized by hue and saturation, plus a white-to-black grayscale column.
- MPU6050 G-meter with smoothing, spike rejection, persistent calibration, history trace, live point, and maximum readings.
- 0–60 mph timer with interpolated threshold crossing and unavailable-speed detection.
- ESP32 Preferences/NVS persistence for gauge selections, colors, favorites, BLE devices, active screen, and G-meter calibration.
- Separate data-fetching task so OBD and sensor polling do not block the main rendering loop.

## Display modes

### Single gauge

A circular needle/arc gauge showing one selected value. The arc and needle use the screen's value color; its label and units use the label color.

### Dual gauge

Two vertical bar gauges displayed side by side. Each side can use a different OBD value. Double-tap the left or right side to change that side directly.

### G-meter

Uses the MPU6050 to display lateral and longitudinal acceleration.

- The current point and numerical maximum values use the value color.
- The history trace uses the label color.
- The circular outline and guide marks use the outline color.
- Implausible spikes are rejected and valid input is smoothed before drawing.
- Numerical readings use `FONT_NORMAL_8`.
- Calibration can be started from **Settings → G-Meter** and is retained after reboot.

Park the vehicle on level ground before starting calibration.

### Acceleration meter

Measures a 0–60 mph run using the vehicle-speed PID.

1. Hold the vehicle stationary for three seconds to arm the meter.
2. Accelerate to begin timing.
3. The final crossing time is interpolated between the two nearest speed samples.
4. Swipe down to reset for another run.

If vehicle speed cannot be read, the displayed speed defaults to `0` and the bottom status reads **Unable to read vehicle speed**. The timer will not arm or advance until a valid speed sample is available. This screen intentionally retains its original fixed color scheme.

### Quadrant gauge

Displays four independently selected values. Labels are centered when they fit; longer labels are centered and ellipsized. Double-tap a quadrant to change only that quadrant's reading.

## Available readings

The selectable command list is defined in `commands.cpp`.

| Reading | Units | Source |
| --- | --- | --- |
| Engine load | % | PID 01 04 |
| Coolant temperature | °F | PID 01 05 |
| Short fuel trim | % | PID 01 06 |
| Long fuel trim | % | PID 01 07 |
| Engine RPM | rpm | PID 01 0C |
| Vehicle speed | mph | PID 01 0D |
| Timing advance | degrees | PID 01 0E |
| Throttle position | % | PID 01 11 |
| Run time | seconds | PID 01 1F |
| Barometric pressure | psi | PID 01 33 |
| Absolute load | % | PID 01 43 |
| Relative throttle | % | PID 01 45 |
| Ambient air temperature | °F | PID 01 46 |
| Engine oil temperature | °F | PID 01 5C |
| Actual engine torque | % | PID 01 62 |
| Transmission temperature | °F | PID 01 05 on the TCM header |
| Engine reference torque | lb-ft | PID 01 63 |
| Boost | psi | Calculated from RPM and engine load |
| Horsepower | hp | Calculated from RPM and torque values |

Calculated values are estimates. A vehicle may not support every direct PID, particularly enhanced powertrain or transmission data.

## Touch controls

| Gesture | Action |
| --- | --- |
| Swipe left | Next gauge screen |
| Swipe right | Previous gauge screen |
| Swipe down | Reset the current gauge |
| Hold for one second | Open the options screen |
| Double-tap | Open the gauge-type shortcut |
| Double-tap a dual-gauge side | Change the selected side |
| Double-tap a quadrant | Change the selected quadrant |
| Swipe right or down in options | Navigate back |

After a type is selected through a double-tap shortcut, the display returns directly to the gauge. Selections made through the full options menu return to that menu.

## Options menu

Holding the display opens the same main menu from every screen:

- **Settings**
  - **Bluetooth:** scan, pair, view connection statistics, select a saved device, or remove a saved device.
  - **G-Meter:** begin MPU6050 calibration.
  - **Device:** reserved for future device settings.
  - **Exit:** return to the main options screen.
- **Gauge Type**
  - **Single Gauge:** select the single-gauge reading.
  - **Dual Gauge:** select the left and right readings.
  - **Exit:** return to the main options screen.
- **Color**
  - **Value Color:** data values, needle/arc, G-meter point, and dual-gauge fill.
  - **Label Color:** labels, units, and the G-meter history trace.
  - **Outline Color:** gauge outlines and guides.
  - **Exit:** return to the main options screen.
- **Exit:** return to the gauge that opened the menu.

### Color selection

Selecting a color role offers two methods:

- **Palette:** choose a hue family, fine-tune hue in 5° steps, then select saturation and brightness. Eight favorite slots can store commonly used colors.
- **Swatches:** choose from eight hue columns and five saturation levels. The top row is fully saturated and each lower row is progressively less saturated. A ninth column provides white, light gray, mid gray, dark gray, and black.

Both methods show a preview and require confirmation. Themes are saved independently for each gauge screen. Favorite slots are shared so a saved color can be reused across screens. The acceleration meter ignores theme changes by design.

## Hardware

The current configuration targets:

- ESP32 DevKitC-V1 or compatible ESP32 board.
- 320×240 TFT supported by `TFT_eSPI`.
- FT6336 capacitive touch controller.
- MPU6050 accelerometer/gyroscope.
- BLE ELM327-compatible OBD-II adapter exposing the configured service and characteristics.

### FT6336 connections

| Signal | ESP32 pin |
| --- | --- |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| Interrupt | GPIO 15 |
| Reset | GPIO 13 |

The MPU6050 normally shares the I²C SDA/SCL bus. TFT pins are selected in the installed `TFT_eSPI` user setup rather than in this repository. Check voltage requirements for every module before wiring; ESP32 GPIO is 3.3 V.

### BLE defaults

The defaults in `config.h` are:

- Service UUID: `FFF0`
- Write characteristic: `FFF2`
- Notify characteristic: `FFF1`
- Initial fallback adapter address: `8c:de:52:dc:5d:2a`

The Bluetooth menu can replace the fallback address with a scanned and saved device. If your adapter uses different UUIDs, update `config.h`.

## Software requirements

- Arduino IDE 2.x or `arduino-cli`.
- Espressif ESP32 Arduino core.
- `TFT_eSPI`.
- `FT6336` touch library.
- Adafruit MPU6050 library.
- Adafruit Unified Sensor library.
- The AudiType GFX font definitions referenced by `config.h`.

`Preferences`, BLE, and FreeRTOS support are supplied by the ESP32 Arduino core.

## Build and setup

1. Install the ESP32 board package and required libraries.
2. Configure `TFT_eSPI` for the connected 320×240 display and its pins.
3. Ensure the AudiType font definitions used in `config.h` are available to the build.
4. Verify the touch mapping and FT6336 pins in `touch.h`.
5. Verify the BLE UUIDs in `config.h` for the intended OBD adapter.
6. Set `TESTMODE` in `ESP32OBDGauge.ino`:
   - `true` allows UI development without making the startup BLE connection.
   - `false` enables normal startup connection to the configured OBD adapter.
7. Select an ESP32 target. The project is currently verified with `esp32:esp32:esp32`.
8. Compile and upload the sketch.
9. Open the serial monitor at 115200 baud for connection and PID diagnostics.

Example command-line build:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

The firmware is close to the default 1,310,720-byte application partition limit. If future additions exceed it, choose a compatible ESP32 partition scheme with a larger application allocation or reduce program size.

## Persistence

The `OBDGAUGE` Preferences namespace stores:

- Last active gauge screen.
- Single, dual, and quadrant reading assignments.
- Independent label, value, and outline colors for each screen.
- Eight shared favorite colors.
- Up to four saved BLE devices and the active BLE address.
- G-meter rotation matrix and bias calibration.

Settings are written when leaving the options workflow or completing an applicable selection, then restored during startup.

## Project structure

| File | Purpose |
| --- | --- |
| `ESP32OBDGauge.ino` | Application setup, tasks, navigation, persistence, and main loop |
| `config.h` | Hardware, display, font, color, and gauge constants |
| `commands.h/.cpp` | OBD command definitions, querying, parsing, calculations, and diagnostics |
| `bluetooth.h/.cpp` | BLE discovery, saved devices, connection, notifications, and ELM327 setup |
| `needle_gauge.h` | Single circular gauge |
| `dual_gauge.h` | Dual vertical gauge |
| `quadrant_gauge.h` | Four-value gauge and quadrant selector |
| `g_meter.h` | MPU6050 G-force display and history |
| `gmeter_calibration.h` | G-meter sample collection and calibration math |
| `acceleration_meter.h` | 0–60 mph timer |
| `options_screen.h` | Unified settings, gauge type, BLE, and color UI |
| `touch.h` | FT6336 input mapping and gesture recognition |
| `screen_manager.h` | Active-screen selection and navigation |
| `ui_library.h` | Shared buttons, tables, and UI primitives |

## Troubleshooting

### The device stays on the G-meter

When the OBD connection is unavailable, navigation falls back to the G-meter because it does not require vehicle data. Confirm that `TESTMODE` is disabled for live operation, the adapter is powered, and the saved BLE address and UUIDs are correct.

### Vehicle values remain at zero

Check the serial output for query status, PID, header, and raw response information. A zero can be a legitimate reading, but timeouts and unsupported PIDs also return a safe default. The acceleration meter separately tracks query validity and shows a warning when vehicle speed is unavailable.

### BLE scanning finds the adapter but pairing fails

Confirm that the adapter exposes service `FFF0` with write characteristic `FFF2` and notify characteristic `FFF1`, or change those values in `config.h`. Some ELM327-compatible products use a different BLE profile or classic Bluetooth and will not work with this configuration.

### Touch positions are offset

Check display rotation and the `TOUCH_MAP_*` values in `touch.h`. The current firmware uses display rotation `3` and maps the FT6336's portrait coordinates to the 320×240 landscape display.

### Custom font build errors

The project references AudiType font symbols such as `AudiType_Normal_038pt7b`. Ensure those GFX font headers are installed and visible in the same Arduino environment used to compile the sketch.

### G-meter readings drift or point in the wrong direction

Park on a level surface and run **Settings → G-Meter → Start Calibration**. The resulting orientation and bias values are saved automatically.
