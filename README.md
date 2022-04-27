This repository aims to document the reverse engineering effort behind Cooler Master Masterkeys keyboards.

As a result you can:
 - re-flash the original firmware with a different key map
 - flash QMK [see here](https://github.com/m-radomski/qmk_masterkeys)
 - write your own bare metal firmware

Currently **only the Masterkeys Pro S White** is tested and proved working.
If you have any other keyboard from this series you can contribute by expanding the current knowledge.
Everything from this point on is written from the point of view of the Pro S White.

# Hardware specification

The keyboard contains three ICs:

 - Holtek HT32F52352 ARM MCU (the chip is probably mislabeled as HT50F52352 [see](https://github.com/pok3r-custom/pok3r_re_firmware/issues/32))
 - GigaDevice GD25Q40C 4MB SPI Flash
 - Macroblock MBIA043GP LED Driver

The MCU is a 48MHz ARM Cortex-M0+ chip.
[ChaoticEnigma](https://github.com/ChaoticEnigma) has already compiled a list of documentation PDFs and examples from Holtek in the [pok3r_re_firmware](https://github.com/pok3r-custom/pok3r_re_firmware) repo.
You are going to see a lot of references to the work of [ChaoticEnigma](https://github.com/ChaoticEnigma), without his RE efforts this probably would not be possible.
Thanks!
The MCU is inside a 64 pin package and almost all of them are used.
The board contains a JTAG connector that will be crucial in flashing your own firmware.
From the start it's locked but you can unlock it which allows you to read/write to it freely.
You can find the pin-out for the JTAG in the schematic stored in the `kicad` directory.
Ground is the pin closest to the MCU, while VDD is the furthest pin from the MCU.
If you feel unsure about it, you can always use a multimeter to confirm.

The SPI is used in the original firmware to store profiles - accessed via `FN + 1-4`.
You can use it in your own firmware if you need, the documentation is inside the `docs` directory, but you can easily find it on the internet.

The LED driver has 16 output pins, it's used in combination with 8 pins from the MCU to form a matrix.
Positive voltage is emitted by the driver to a particular column and the LED will glow if the row has been pulled low by the MCU.
Currently the LED driver is not used in the QMK firmware, but work is being done on getting it working!
Documentation for this driver is hard to get but [ChaoticEnigma](https://github.com/ChaoticEnigma) and this [post](https://www.reddit.com/r/AskElectronics/comments/db348y/failling_to_find_mbia043gp_mbia043gp_b0n639cgha/) give us the answers.
Since MBI5040 chip is similar I'm storing it's documentation inside the `docs` directory, but you can get the specific documentation from the [pok3r_re_firmware](https://github.com/pok3r-custom/pok3r_re_firmware) repo.

# Original firmware

_I do not take responsibility for any damages that you might make to your keyboard while trying to anything with it_

From the three Pro X White variants only Pro M and Pro L have firmware upgrades.
This makes the process of recovering the original Pro S White firmware hard.
Cooler Master hosts these updates [here for Pro M](http://update.coolermaster.com/masterkeys-pro-m-white/masterkeys-pro-m-white-v1.06.zip) and [here for Pro L](update.coolermaster.com/masterkeys-pro-l-white/masterkeys-pro-l-white-v1.08.zip).
Using the [pok3rtool](https://github.com/pok3r-custom/pok3rtool) you can extract the firmware from these updaters which has [already been discussed in this issue](https://github.com/pok3r-custom/pok3rtool/issues/6).

If you plan to flash your own firmware and your bootloader is not currently inside the `binaries` directory, I advise to flash a modified version of the firmware with the flash reading patch.
It's pretty well described in the issue linked above.
After patching and dumping the `bootloader + firmware` you are more or less safe - meaning if you manage to brick your keyboard you can reprogram it using any JTAG debugger or Raspberry Pi, I used the Raspberry Pi Zero which worked a treat.
You if you dumped a bootloader from a different model your contribution is welcomed!

## Pro S White note

Since firmware for this keyboard is not provided by Cooler Master currently it's not possible to re-flash back to stock firmware.
You can use Pro L firmware and modify - using a hex editor - the key map which brings your keyboard back to a working condition.
As a side effect **lighting no longer works**!
If you want to modify the key map it's defined as an array of USB-HID codes starting at offset `0x618d` in the Pro L firmware file.
In the `binaries` directories I attached a Pro L firmware with the Pro S keymap - Caps is Escape right now.

# Flashing

## The role of the original bootloader

The keyboard has essentially two programs on it: bootloader and firmware.
Firmware is what is running on it most of the time, everything from lighting to profiles are implemented there.
The thing I refer to as a bootloader is an in-application programming (IAP) program.
It allows you to reprogram the firmware without having to use the JTAG connector every time.
So if you want to quickly iterate over firmwares I'd suggest leaving the bootloader be.

You have two options of flashing:

## Erase the original bootloader and program every time using the JTAG

If you want to need more flash memory you can remove the original bootloader which takes around 12.5Kb (~10% of the flash).
It's almost never the case you want to do this so move to the second option.

## Use the bootloader to write the new firmware using the pok3rtool

The already mentioned [pok3rtool](https://github.com/pok3r-custom/pok3rtool) is perfect for flashing the new firmware.

If you stick to the original firmware and only make small tweaks it will put the keyboard in to the bootloader mode for you and reprogram it.

On the other hand if you use QMK you can set your keyboard so that it will boot into the bootloader.
This is already done in the linked QMK, just press `LShift + RShift + B`.
Your keyboard will boot into the bootloader and await being programmed.
You can confirm this by watching the output `dmesg`, it should appear as a `Cooler Master HID-IAP`.
When it's ready flash the firmware.

## Example flashing using the bootloader

 - Download [pok3rtool](https://github.com/pok3r-custom/pok3rtool)
 - Apply the patch referencing your keyboard from `patches`
 - Compile the tool
 - Run `sudo ./pok3rtool --ok -t cmpro<s|m|l> flash <version string> <your firmware binary>`
 - Your keyboard is flashed!
 
# Unlocking the MCUs JTAG

[ChaoticEnigma](https://github.com/ChaoticEnigma) already figured this out [here](https://github.com/pok3r-custom/pok3r_re_firmware/wiki/HT32-Unlocking).
His fork of the OpenOCD with the HT32F1xxxx support works with HT32F5xxxx MCUs as well.
Remember that the logic of the keyboard works on `3.3V`!
If you want to be extra careful not to fry anything just connect all cables without VDD and power the keyboard by connecting it to USB.
