# RPi_Baremetal_SDCARD_SPI

## What's this?
This is a demo of baremental SD CARD access from Raspberry Pi

## Anything new from other work?
It uses SPI mode, in contrary to sd mode (or bus mode).
Also it accesses SD CARD through GPIO, instead of EMMC on Raspberry Pi

## What's good about that?
Well, Raspberry Pi's EMMC isn't so well documented. BCM2835-ARM-Peripherals Doc says that the user needs to refer to the document from IP suplier, Arasan, but the document is not open to public.
OK, there are multiple Baremetal projects that uses SDCARD beautifully, it is a result of guessing and reverse engineering. The engineers who did that are all great, but for the rest of us, how to access SDCARD from baremetal environment isn't so clear yet.
However, by using GPIO and accessing SDCARD directly, we can skip those mysteries of EMMC, and can directly deal with SDCARD interface, which is mostly well documented.

## Any drawback?
Many.
- It's very slow because GPIO is controlled by SW
- Unstable. I haven't implemented all the functions necessary to support all kinds of SDCARD.
- You need to remove and reinsert the card before doing this. This is because the bootloader is loaded from SDRAM in SD mode, and I was never able to make it run in SPI mode after that. You need to reset the card, which can be only done by removing the ard.

## Damn, then, this is useless
Practically, yes. 

## Then, what's the point of this project?
Mostly educational. 
You can see how SD CARD interacts with the host almost directly, by using GPIO and controlling every bits your self.
Also, if you want to add an extra SDCARD slot using GPIO, you can do the same thing, and add more.
Oh, also, since it's all SW, you have chance of extending the capability beyond the current SDHC/SDXC.

## What does this demo do?
It shows that it can indeed read and write sectors.
Then, it shows the root directly of FAT32 partition.
It worked on my SDCARD. It may work on yours or may not. It's just a demo.

## what is the next plan?
I want to access SDCARD in SD mode, so that you don't have to remove and reinsert the card.
Then, I want to combined this work to another project of mine, RPiHaribote.

# How to run

## Pre-requisite

This runs on Raspberry Pi B+. It probably runs on Raspberry Pi Zero (W), but I havent' confirned yet.

You need tool chain (arm-none-eabi-gcc, arm-none-eabi-make) on your host PC. I assume you run Linux, in particular Ubuntu.
On Ubuntu, you should be able to install them by

```bash
sudo apt install gcc-arm-none-eabi
```

You also need UART connected to pin 14 and pin 15 of Raspberry Pi, and need a terminal software to see the output from the demo.

Also, you need a SDCARD. It doesn't have to be big, but needs to be of SDHC. You also need to install Raspbian OS, because the demo reuses raspbian's bootloader.
I strongly recommend to *use a SDCARD you don't use for regular work* and you are OK with breaking the content, because one of the demo will write to SD CARD.
Although the program try to retrieve the content back, there's no guarantee that it works.

## How to build and run
Clone or copy the project and build by ```Make```
Rename the kernel.img file in the target SDCARD as a back up, and copy the newly build kernel.img to the sdcard.
Connect your PC's UART to Pin 5 (GND), Pin 7 (RX), and Pin 9 (TX).
Then turn on Raspberry Pi.
You should be seeing a demo of SDCARD read/write, and showing the file names and sizes on the root directory of the FAT partition.

