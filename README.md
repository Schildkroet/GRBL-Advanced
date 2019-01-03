***
Grbl-Advanced is a no-compromise, high performance, low cost alternative for CNC milling. This version of Grbl-Advanced runs on a STM32F411 Nucleo Board.

It accepts standards-compliant g-code and has been tested with the output of several CAM tools with no problems. Arcs, circles and helical motion are fully supported, as well as, all other primary g-code commands. Macro functions, variables, and most canned cycles are not supported, but we think GUIs can do a much better job at translating them into straight g-code anyhow.

Grbl-Advanced includes full acceleration management with look ahead. That means the controller will look up to 16 motions into the future and plan its velocities ahead to deliver smooth acceleration and jerk-free cornering.

* Built on the wonderful Grbl v1.1 (2017) firmware written by Sungeun "Sonny" Jeon, Ph.D. (USA).

***

#### Backlash Compensation:
Backlash compensation enabled by default. You can switch it off in Config.h.
Edit defaults.h to your needs.

* $140=(X Backlash [mm])
* $141=(Y Backlash [mm])
* $142=(Z Backlash [mm])

#### Canned Drill Cycles (G81-G83):
Added Canned Drill Cycles G81-G83 as experimental features. They are roughly tested and seem to work, but may contain still errors. Use at own risk!


### Build Environment:

[EmBitz 1.11](https://www.embitz.org/)

### Hardware:

* [STM32 Nucleo F411RE](http://www.st.com/en/evaluation-tools/nucleo-f411re.html)
* STM32F411RET6 in LQFP64 package
* ARM®32-bit Cortex®-M4 CPU with FPU
* 96 MHz CPU frequency
* 512 KB Flash
* 128 KB SRAM

***
### Install:

#### Windows
* Download and install EmBitz
* Open .ebp Project File with EmBitz
* Select 'Release' Target
* Hit Compile
* Flash HEX created in bin/Release

#### Linux
* Install Build Essentials, [GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm) and [texane st-util](https://github.com/texane/stlink)
* Run 'make' and 'make flash' in Terminal

***

```
List of Supported G-Codes in Grbl-Advanced:
  - Non-Modal Commands: G4, G10L2, G10L20, G28, G30, G28.1, G30.1, G53, G92, G92.1
  - Motion Modes: G0, G1, G2, G3, G38.2, G38.3, G38.4, G38.5, G80
  - Canned Cycles: G81, G82, G83
  - Feed Rate Modes: G93, G94
  - Unit Modes: G20, G21
  - Distance Modes: G90, G91
  - Arc IJK Distance Modes: G91.1
  - Plane Select Modes: G17, G18, G19
  - Tool Length Offset Modes: G43.1, G49
  - Cutter Compensation Modes: G40
  - Coordinate System Modes: G54, G55, G56, G57, G58, G59
  - Control Modes: G61
  - Program Flow: M0, M1, M2, M30*
  - Coolant Control: M7*, M8, M9
  - Spindle Control: M3, M4, M5
  - Valid Non-Command Words: F, I, J, K, L, N, P, R, S, T, X, Y, Z
```
