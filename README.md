##Radio Music

### A virtual radio module for Eurorack (synchronized)
![Image from matcsat, Muffwiggler forum](https://raw.githubusercontent.com/TomWhitwell/RadioMusic/master/Collateral/BuildImages/rmpic.jpg)

Based on the [original firmware](https://gitter.im/TomWhitwell/RadioMusic) by Tom Whitwell

Allows to play files synchronously with clock.

Usage:
- put 16-th trigger stream into the Station input
- set BPM parameter to your desired base tempo in settings.txt on the SD card (default 130)
- upload audio loops with the desired tempo (the one you set in settings.txt)

If your loops match desired base tempo (BPM setting in settings.txt) - playback will follow tempo of incoming 16-th trigger

WARNING: this is an alpha version, so please download the original firmware beforehand. There are more things to implement, so please be patient.

Demo: https://vimeo.com/187965734

Latest Firmware: http://www.mediafire.com/file/4hton1xd45stxga/RadioMusic.ino.hex
