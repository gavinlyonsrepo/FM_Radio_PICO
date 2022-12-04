[![Website](https://img.shields.io/badge/Website-Link-blue.svg)](https://gavinlyonsrepo.github.io/)  [![Rss](https://img.shields.io/badge/Subscribe-RSS-yellow.svg)](https://gavinlyonsrepo.github.io//feed.xml)  [![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/whitelight976)

Table of contents
---------------------------

  * [Table of contents](#table-of-contents)
  * [Overview](#overview)
  * [Hardware](#hardware)
  * [Software](#software)
  * [Schematic](#schematic)
  * [Output](#sutput)


Overview
-------------------
* Name: FM_Radio_PICO
* Description:  FM stereo radio project for the RPI PICO.
* Developed on and for
	1. Raspberry pi PICO RP2040
	2. SDK C++ compiler G++ for arm-none-eabi
	3. CMAKE , VScode

Hardware
-------------------

The Radio Module used is a TEA5767NH. An OLED CH1115 is used as a screen
for output. 3 push buttons are used for control (Mute ,Search up radio band ,Search down radio band). The ADC is used to read the output of the Audio Amplifier which is connected to Audio Out of the Radio module thru a voltage divider.
This gives us a volume indicator.  An AHT10 sensor is read 
and it's temperature and humidity data is displayed at bottom of screen.
As the TEA5767 is a 5 volt logic device(and PICO GPIO is 3.3V)  I connect to it through a voltage level inverter. I had to use two separate buses for both I2C devices as they would not work reliably on same bus. The on-board PICO Status LED is used.

 ![image ](https://github.com/gavinlyonsrepo/TEA5767_PICO/blob/main/extra/images/radio.jpg)

Software
-------------------
The libraries used are included. They can all also be found in stand alone format on my PICO projects list page [here. ](https://github.com/gavinlyonsrepo/RPI_PICO_projects_list)

Debug information can be printed to serial console (38400 baud) by changing setting.
The TEA5767 also has its own built-in separate debug prompt which can also be enabled
in the library. 


Schematic
-------------------

1. TODO


Output
------------------------
![ image ](https://github.com/gavinlyonsrepo/FM_RADIO_PICO/blob/main/extra/images/radiodata.jpg)
 

