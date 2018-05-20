# RPi_SDCARD_Baremetal_SPI

## What's this?
This is a demo of baremental SD CARD access from Raspberry Pi

## Anything new from other work?
It uses SPI mode, in contrary to sd mode (or bus mode) contrary to the regular Raspberry Pi OS uses.
Also it accesses SD CARD through GPIO, instead of EMMC on Raspberry Pi, with the control by software.

## What's good about that?
Well, Raspberry Pi's EMMC isn't so well documented. BCM2835-ARM-Peripherals Doc says that the user needs to refer to the document from IP suplier, Arasan, but the IP vender does not share the IP doc with hobby users.  
OK, there are already multiple Baremetal projects that uses SDCARD beautifully, it is a result of guessing and reverse engineering. The engineers who did the job are all great, but for the rest of us, how to access SDCARD from baremetal environment remains obscure.  
However, by using GPIO and accessing SDCARD directly, we can skip those mysteries of EMMC, and can directly deal with SDCARD interface, which is mostly well documented.

## Any drawback?
Many.

- It's very slow because GPIO is controlled by SW

- Unstable. I haven't implemented all the functions necessary to support all kinds of SDCARD

- You need to remove and reinsert the card before using this. This is because the bootloader is loaded from SDRAM in SD mode, and I was never able to make it run in SPI mode after that. You need to reset the card, which can be only done by removing the card

## Damn, then, this is useless
Practically, yes. 

## Then, what's the point of this project?
Mostly educational.  
You can see how SD CARD interacts with the host almost directly, by using GPIO and controlling every bits your self.
Also, if you want to add an extra SDCARD slot using GPIO, you can do the same thing, and add more.
Oh, also, since it's all SW, you have chance of extending the capability beyond the current SDHC/SDXC.

## What does this demo do?
It shows that it can indeed read and write sectors.
Then, it shows the files in the root directly of FAT32 partition.  
It worked on my SDCARD. It may work on yours or may not. It's just a demo.

## what is the next plan?
I want to access SDCARD in SD mode in baremetal, again through GPIO, so that you don't have to remove and reinsert the card.
Then, I want to combined this work to another project of mine, RPiHaribote.

# How to run the demo

## Pre-requisite

This runs on Raspberry Pi B+. It probably runs on Raspberry Pi Zero (W), but I havent' confirned yet.

You need tool chain (arm-none-eabi-gcc, arm-none-eabi-make) on your host PC. I assume you run Linux, in particular Ubuntu.
On Ubuntu, you should be able to install the tool chain by

```bash
sudo apt install gcc-arm-none-eabi
```

You also need UART connected to pin 14 and pin 15 of Raspberry Pi, and need a terminal software to see the output from the demo.

Lastly, you need a SDCARD. It doesn't have to be big, but needs to be of SDHC. You also need to install Raspbian OS, because the demo reuses raspbian's bootloader.
I strongly recommend to **use a SDCARD you are OK with breaking**, because one of the demo will write to SD CARD.
Although the program try to retrieve the content back, there's no guarantee that it works.  
Whatever happens to your SDCARD, the author provides **no warranty**.

## How to build and run
Clone or copy the project to your host PC.  
Build it with ```Make```  
Rename the kernel.img file in the target SDCARD as a back up, and copy the newly built ```kernel.img``` to the sdcard in the folder where the old kernel.img used reside.  
Connect your PC's UART to Pin 5 (GND), Pin 7 (RX), and Pin 9 (TX).  
Then turn on Raspberry Pi.  
You should be seeing a demo of SDCARD read/write, and showing the file names and sizes on the root directory of the FAT partition.

