![GitHub Logo](https://github.com/Schildkroet/GRBL-Advanced/blob/software/doc/en.nucleo-F4.jpg?raw=true)

***
Grbl-Advanced is a no-compromise, high performance, low cost alternative for CNC milling. This version of Grbl-Advanced runs on a STM32F411 Nucleo Board.

It accepts standards-compliant g-code and has been tested with the output of several CAM tools with no problems. Arcs, circles and helical motion are fully supported, as well as, all other primary g-code commands. Macro functions, variables, and most canned cycles are not supported, but we think GUIs can do a much better job at translating them into straight g-code anyhow.

Grbl-Advanced includes full acceleration management with look ahead. That means the controller will look up to 16 motions into the future and plan its velocities ahead to deliver smooth acceleration and jerk-free cornering.

* Built on the wonderful Grbl v1.1 (2017) firmware written by Sungeun "Sonny" Jeon, Ph.D. (USA).

***

### Extras:
#### Backlash Compensation:
Backlash compensation enabled by default. You can switch it off in Config.h.
Edit defaults.h to your needs.

* $140=(X Backlash [mm])
* $141=(Y Backlash [mm])
* $142=(Z Backlash [mm])

#### Canned Drill Cycles (G81-G83):
Added Canned Drill Cycles G81-G83 as additional features. 

#### 5-Axis support
Added experimental support for rotary axis (5-axis). They are roughly tested and may contain still errors. Use at own risk!

#### Hard Reset
0x19 (CTRL-Y): Perform a hard reset.

#### Tools
* $14=(tool change mode): 0 = Ignore M6; 1 = Manual Tool Change; 2 = Manual Tool Change + TLS; 3 = Tool Table
* $P: Save TLS position
* $T: Confirm tool change
* $Tx: Print parameters of Tool Nr x (Supports Tool Nr 0-19)
* $Tx=[0.0:0.0:0.0:0.0] (Save new parameters of Tool x: X, Y, Z, Reserved)
* $RST=T: Reset all tool tables saved in EEPROM

Uses Dynamic TLO when $14 = (2 or 3)

#### I2C EEPROM
Added support for external EEPROM (e.g. ST M24C08). Uncomment 'USE_EXT_EEPROM' in Config.h.
![EEPROM](https://github.com/Schildkroet/GRBL-Advanced/blob/software/doc/eeprom.png?raw=true)

#### ETHERNET Support
GRBL-Advanced can be controlled with USB or ETHERNET. For ETHERNET an additional W5500 Module is required. Then uncomment ETH_IF in Platform.h. The default IP Address is 192.168.1.20.
Use [Candle 2](https://github.com/Schildkroet/Candle2) as control interface.
![W5500](https://github.com/Schildkroet/GRBL-Advanced/blob/software/doc/w5500.png?raw=true)

#### Attention
By default, settings are stored in internal flash memory in last sector. First startup takes about 5-10sec to write all settings.

***

### Build Environment:

[EmBitz 1.11](https://www.embitz.org/)

### Hardware:

* [STM32 Nucleo F411RE](http://www.st.com/en/evaluation-tools/nucleo-f411re.html)
* STM32F411RE in LQFP64 package
* ARM®32-bit Cortex®-M4 CPU with FPU
* 96 MHz CPU frequency
* 512 KB Flash
* 128 KB SRAM

* [STM32 Nucleo F446RE](https://www.st.com/en/evaluation-tools/nucleo-f446re.html)
* STM32F446RE in LQFP64 package
* ARM®32-bit Cortex®-M4 CPU with FPU
* 168 MHz CPU frequency
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
* Download [GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm) and unpack it to /opt. In makefile update path to toolchain.
* Run following commands:
```
sudo apt install build-essential stlink-tools
```
* Clone repository and run following commands:
```
make clean
make all flash
```

***

```
List of Supported G-Codes in Grbl-Advanced:
  - Non-Modal Commands: G4, G10L2, G10L20, G28, G30, G28.1, G30.1, G53, G92, G92.1
  - Motion Modes: G0, G1, G2, G3, G38.2, G38.3, G38.4, G38.5, G80
  - Canned Cycles: G81, G82, G83
  - Feed Rate Modes: G93, G94
  - Unit Modes: G20, G21
  - Distance Modes: G90, G91
  - Retract Modes: G98, G99
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
