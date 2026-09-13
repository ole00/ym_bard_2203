# ym_bard_2203
A dual YM2203 sound chip board for Arduino Nano
![Board image](https://github.com/ole00/ym_bard_2203/raw/master/img/ym_bard_2203.jpg "ym_bard_2203")

This is a sound board with two YM2203 FM sound chips driven by Arduino Nano. The YM2203 FM chips were used 
in several arcade game boards made in 80's. The design of the YM Bard sound board was actually based on the
schematic of an arcade game.

The board allows you to experiment with YM2203 sound chips by writing the chip's register values
from a PC over a serial port. It is also possible to stream an arcade game sound data over
a serial port from a PC based emulator and produce authentic sound and music of an arcade game.

YM2203 chips are register compatible with AY-3 PSG sound generator, therefore you can use FM Bard
to play AY-3 tunes as well.

Arduino selection
----------------------

The b(o)ard requires and Arduino Nano or a NANO clone. You can use 5V Atmel based Arduino Nano
or 3.3V ESP32-S3 based Arduino NANO or its clone. The picture above uses a cheap LGT8F328P 
Nano board.

YM2203 requires an external clock of specific frequency to produce sounds. The clock can 
be generated either by Arduino NANO or by an external Si5351 module connected on the bottom
side of the YM Bard. The Si5351 module is optional, however the Atmel based Arduino NANO can
only produce limited selection of clock frequencies, so it is recommended to use the module 
in such case. ESP32-S3 based Arduino NANO can produce various frequencies and the clock module
is not required.

Audio data streaming
----------------------
To stream data to YM Bard use the Arduino sketch 'ym2203_streamer.ino' located in 'arduino-src'
subdirectory. The sketch reads binary data sent from the connected PC serial port (115200 baud rate)
and writes the data to the parallel data bus of the FM chips. The stream format supports either 
3 byte packet and/or 12 byte packet.

**3 byte packet**

* 1st byte: bits 7-4: 0xF, bit 3: 0, bits 2-0: chip selection (0 selects the 1st YM2203, 1 selects the 
  2nd YM2203).
* 2nd byte: register address
* 3rd byte: register value

**12 byte packet**
* 1st byte: bits 7-4: 0xF, bit 3: 1, bits 2-0: chip selection (0 selects the 1st YM2203, 1 selects the 
  2nd YM2203).
* byte 2 to byte 12: register values of register 0 to register 10.

3 byte packet allows you to write single arbitrary register. It is flexible, but requires
more data per register to be sent compared to 12 byte packet. 12 byte packets basically sends
multiple PSG registers in one go, saving the serial line bandwidth.



