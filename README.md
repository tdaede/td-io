td-io is a JVS I/O board designed to adapt older JAMMA-compatible cabinets to the newer JVS standard, used by modern arcade hardware such as the Naomi and exA. It features a number of independent systems to translate signals, drive cabinet hardware, and power JVS games.

# Features at a glance

* Chainable JVS I/O implementing a 2L12B panel and coin inputs
* JVS to JAMMA video converter compatible with all monitors
* Mono/stereo audio amplifier for driving JAMMA speakers
* JVS power connector providing 3.3V via an on-board converter
* High reliability, low heat generation and all solid state caps
* Upgradeable firmware

# Comparison with other JVS I/O options

|               | TD-IO   | Capcom I/O   | Sega 838   | Exa I/O |
| ------------- | ------- | ------------ | ---------- | ------- |
| Lag (frames)  | 0       | 1            | 0          | 0       |
| Chaining      | ✔ | ✔ | ✔ | ❌ |
| Video         | ✔ | ✔ | ✔ | ❌ |
| Audio amp     | ✔ | ✔ | ❌ | ❌ |
| 3.3V power    | ✔ | ✔ | ❌ | ❌ |
| Amplified stereo | ✔ | ❌ | ❌ | ❌ |

# JVS inputs

The JVS I/O bus connects to the board via USB-style A and B connectors. Daisy chaining is supported. The I/O presents two players, each with a single joystick/lever and six buttons. Joystick and button 1-4 inputs are mapped to the JAMMA edge connector, and buttons 4-6 are mapped to a CPS2-style kick harness connector. Buttons are polled synchronously, ensuring microsecond-level latency.

Service, test, and tilt buttons are also supported. JAMMA's single service button is mapped to JVS player 1's service button.

The board also presents one or two coin slots, toggleable by a DIP switch. The board also features two coin meter drivers and two coin lockout drivers. Coins are locked out when the respective coin count is set to its max value (16383).

# Video

The board maps JVS-level video (0.7Vp-p, AC or DC coupled) to JAMMA levels (0-3V, DC coupled). This is achieved via a DC restore step using the sync signal, followed by a fixed amplification. The DC coupled output allows maximum compatibility with monitors such as the Nanao MS8 and capture devices such as the Splitfire which require it.

In addition, a sync combiner combines negative H/V sync to negative composite sync.

No rate conversion is performed - the monitor must support the same scan rates as the JVS game.

An EDID EEPROM is also attached to the VGA port, to identify as a 31kHz monitor.

# Audio

TD-IO provides a class-D audio amplifier, which can drive both mono and stereo speakers in JAMMA cabinets. For mono cabinets, audio is simply provided via the JAMMA edge. For stereo support, the cabinet's speakers should be connected to the JST NH header provided on the I/O. The pinout of this header is directly compatible with the Egret 2, and adapters can be easily made for other cabinets, such as the Aero.

The stereo/mono switch selects whether to mixdown the L/R channels to mono. It should be set to mono when using a mono cabinet via the JAMMA edge, and stereo when using a cabinet with the stereo header.

The stereo support is not directly compatible with nonstandard Neo Geo style stereo over JAMMA, with a common ground. For these cabinets, either use the TD-IO in mono mode, or rewire the cabinet to connect directly to the stereo header. The left/right negative speaker outputs should not be tied together.

# Power

An 8-pin JVS power connector forwards 12V and 5V from the cabinet to the JVS board. In addition, an on-board power supply generates 3.3V at up to 7A from the cabinet's 5V rail for systems that require it, such as Naomi.

The cabinet's 5V power supply must still be adequate to power the 3.3V converter.

The IO's electronics are protected from overvoltage and overcurrent by an automatically resetting protector.

# Firmware Updates

The board features an on-board RP2040 microcontroller, whose firmware can be updated over USB for feature enhancements or compatibility fixes.

To update the firmware, turn off power to the cabinet (if attached). Then, attach a USB cable to the micro USB connector. Hold down the white button on the RP2040 while plugging the USB connector into a PC. The TD-IO will then appear as a flash drive. Copy the firmware file (ending in .uf2) onto the drive. The TD-IO will then disconnect and be ready for use.

# Analog Input

The board features an unpopulated expansion header exposing 3 3.3V level analog inputs, designed for potentiometers for analog joystick or racing games. This feature is experimental and not yet implemented in the firmware.

# Where to buy

The board is not currently for sale, as testing is not yet complete.

# License

CERN-OHL-W
