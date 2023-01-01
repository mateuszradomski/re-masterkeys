# MBIA043 RE notes:

Full credit: [hansemro](https://github.com/hansemro).

Original issue [here](https://github.com/mateuszradomski/re-masterkeys/issues/1).

These results were discovered from firmware disassembly and testing in QMK.

QMK test branch: https://github.com/hansemro/qmk_firmware/tree/prosrgb_mbia043_testing2
QMK (soon)PR branch: https://github.com/hansemro/qmk_firmware/tree/prosrgb_mbia043_dev

## Instructions
Instructions are determined by the number of DCLK rising edges while LE is asserted.

For instructions that read data from shift register, the falling edge of LE must occur shortly after the final data entry gets shifted.

### Data Latch: 1 DCLK rising edge

Moves data from shift register to buffer.

### Overall Latch: 2 DCLK rising edges

Moves data in buffers to comparators.

### (Undocumented) Read Configuration: 4 DCLK rising edges

Moves 10-bit configuration data to shift register.

Configuration:
```
  MSB     ...     LSB
0b0_0_0_0_0_0_0_0_1_0 (default)
  | | | | | | | | | `-> counter disable (when 1)
  | | | | | | | | `-> unk1
  | | | | | | | `-> unk2
  | | | | | | `-> unk3
  | | | | | `-> unk4 (not saved)
  | | | | `-> unk5 (not saved)
  | | | `-> unk6 (not saved)
  | | `-> unk7 (not saved)
  | `-> unk8
  `-> unk9 (not saved)
```

### (Undocumented) Enable Write Configuration: 18 DCLK rising edges

Enables Write Configuration instruction (if it is the next instruction).

Used by v122 firmware.

### (Undocumented) Write Configuration: 8 DCLK rising edges

Moves data from shift register to (undocumented) configuration register.

Used by v122 firmware to configure each MBIA IC to 0b0000001100.

## Routines

### Shifting data

On every rising edge of DCLK, the shift-register shifts in data from SDI and out of SDO. This includes cases when LE is asserted high (to call an instruction using data in the shift-register).

v122 `mbia_shift_data`: https://github.com/hansemro/pok3r_re_firmware/blob/8346ba6d6a77e60094a02dcac3b5c8559cb7bf75/disassemble/cmprosrgb/v122/cmprosrgb_v122.txt#L13111-L13145

### Sending an instruction

Set LE high until the requisite number of DCLK rising edges has been met, then set LE low. Whatever data is in the shift-register on the final DCLK edge is used for the instruction (and not a DCLK cycle later).

v122 `mbia_send_instruction`: https://github.com/hansemro/pok3r_re_firmware/blob/8346ba6d6a77e60094a02dcac3b5c8559cb7bf75/disassemble/cmprosrgb/v122/cmprosrgb_v122.txt#L13075-L13108

### Setting grayscale values

A grayscale value for each channel must be set (starting from channel 16/OUT15) when updating comparator values. For each channel, a (10-bit) grayscale data is shifted in with LE asserted only for the last bit (Data Latch). After latching data for the final channel, send Overall Latch command.

Note that cascading MBIAs essentially become an extended shift-register, so the same general steps above apply to this case but with N * 10-bit data to shift for each channel.

v122 mbia_shift_RGB_from_sram (writes RGB data to buffers for single row): https://github.com/hansemro/pok3r_re_firmware/blob/8346ba6d6a77e60094a02dcac3b5c8559cb7bf75/disassemble/cmprosrgb/v122/cmprosrgb_v122.txt#L8413-L8490

v122 bftm1_intr (commits buffers to comparators): https://github.com/hansemro/pok3r_re_firmware/blob/8346ba6d6a77e60094a02dcac3b5c8559cb7bf75/disassemble/cmprosrgb/v122/cmprosrgb_v122.txt#L657-L708

### Timing

For MBIA043 at 3.3V, DCLK has maximum allowed frequency of 25 MHz, so use the appropriate number of NOP delays to operate below this limit. (e.g. For ARM core running at 72 MHz, 3 NOPs are required to run at/below 24 MHz).
