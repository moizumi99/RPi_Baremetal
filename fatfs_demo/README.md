# About
This folder contains the monitor program of FATFS using SDCARD interface for Raspberry Pi.
This monitor program is ported from the H8 version built by ChaN, the author of FatFS, and copy rights belongs to ChaN too.
This is only a demo program at this moment, and doesn't support all commands or configuration. Stil, it can do some tricks.

# Prerequisite
- Raspberry Pi B+
- UART connection to mini uart port of Raspberry Pi B+ (GPIO pin 14 and 15)
- SDHC card with Raspbian installed
Please make sure you use SDHC card that you don't need and does not have any important file. The author provide no warranty with whatever consequence the software brings.

# how to use
- Install Raspbian. Make sure Raspberry Pi boots on the SDHC card.
- Compile the program in this repository by "make" command
- Copy kernel.img to sdcard's boot section replacing old kernel.img file
- Boot Raspberry Pi with the SDCARD
- Prompt will show up on the UART monitor

# Commands
Current the following commands are supported

di 0 : initialize the sdcard
dd 0 n : dump the content of the sdcard starting from sector n
ds 0 : show disk status of the sdcard

fi 0 : mount sdcard file system. The followin fx commands need fi 0 first.
fs : shows the file system information
fl [<path>] : list the files and directories
fo flag filename : open a file
fd size : dump the content of the currently open file up to "size" bytes
fc : close the current file

There are more commands available but not validated yet.
You can check the list of the commands by typing "?"

