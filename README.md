# RPi_Baremetal

## What's this?
This is a respository of my Raspberry Pi Baremetal Experiments

## Contets
* sdcard_gpio_spi: demo of accessing sdcard through gpio in SPI mode

* sdcard_gpio_bus: demo of accessing sdcard through gpio in bus mode (standard mode)

## Running the demo
* Prepare a sd card that holds raspbian OS and you __don't need__ any more (Contents of the card may be destroyed by this demo. So, make sure to use a one that you can waste.)

* replace kernel.img in your raspberry pi boot sd card with the kernel.img here, and start raspberry pi

* You need UART connection to GPIO ports #14 and #15



