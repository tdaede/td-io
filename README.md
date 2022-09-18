# smol-jvs-io

Smol JVS IO is a scoped down JVS I/O board designed to work as a drop-in replacement for arcade stick PCB's, to allow them to interface with JVS hardware. It is not meant to function as a fully functional JVS IO in terms of functionality.

This hardware and software of this board is forked from [td-io](https://github.com/tdaede/td-io) and then heavily scoped down to fit my needs. The td-io firmware is not directly compatible, due to design shortcuts on this board such as removing the shift registers.

# Features

* JVS IO board interface as one-player device.
* Brook board compatible 20pin connector for hooking up player 1 stick and buttons.
* Upgradeable firmware

# Stuff intentionally not provided

* Player 2 controls
* Chaining other JVS IO boards
* Video/Audio handling
* Analog controls, Mahjong controls...

# Power

Power can either be provided to the micro USB port of the raspberry pi pico, or through the +5V/GND pins of the board.

# Firmware Updates

Flash the Raspberry Pi Pico with new firmware. Make sure stick is not powered from other sources.

# License

CERN-OHL-W
