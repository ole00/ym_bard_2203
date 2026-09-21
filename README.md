# ym_bard_2203
A dual YM2203 sound chip board for Arduino Nano
![Board image](https://github.com/ole00/ym_bard_2203/raw/master/img/ym_bard_2203.jpg "ym_bard_2203")

This is an open source/open hardware sound board with two YM2203 FM sound chips driven by Arduino Nano. 
The YM2203 FM chips were used in several arcade game boards made in 80's. 
The design of the YM Bard sound board was actually based on a schematic of an arcade game.

The board allows you to experiment with YM2203 sound chips by writing the chip's register values
from a PC over a serial port. It is also possible to stream an arcade game sound data over
a serial port from a PC based emulator and produce authentic sound and music of an arcade game.

YM2203 chips are register compatible with AY-3 PSG sound generator, therefore you can use FM Bard
to play AY-3 tunes as well.

Arduino selection
----------------------

The b(o)ard requires and Arduino Nano or a NANO clone. You can use 5V Atmel based Arduino Nano
or Renesas based Arduino NANO R4 or 3.3V ESP32-S3 based Arduino NANO or their clones.
The picture above uses a cheap LGT8F328P Nano board.

YM2203 requires an external clock of specific frequency to produce sounds. The clock can 
be generated either by Arduino NANO or by an external Si5351 module connected on the bottom
side of the YM Bard. The Si5351 module is optional, however the Atmel based Arduino NANO can
only produce limited selection of clock frequencies, so it is recommended to use the module 
in such case. NANO R4 and ESP32-S3 based Arduino NANO can produce various frequencies 
and the clock module is not required for driving the FM clock at 3 MHz.

Audio data streaming
----------------------
To stream data to YM Bard use 'ym2203_streamer.ino' Arduino sketch located in 'arduino-src'
subdirectory and upload it to your Arduino NANO. Use Arduino IDE PC software to upload the sketch.
If you use Arduino NANO ESP32-S3 then make sure to install "esp32 by Espressif" version 3.3.X
board support in Arduino IDE Board manager before you do the upload.

The sketch reads binary data sent from the connected PC serial port (115200 baud rate)
and writes the data to the parallel data bus of the FM chips. The stream format supports two types of packets: 
3 byte packet and 12 byte packet.

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

To start streaming open the serial port of your Arduino NANO and write the packets to
the serial port in binary format.

Example playback
-----------------
If you use Linux you can try to connect YM Bard to Ghost'n Goblins arcade emulator located here:

https://github.com/ole00/arcade_emu_gng

Pass the serial port device of the YM Bard to the emulator with parameter '-fm [device name]'
when starting the emulator. For example add '-fm /dev/ttyUSB0' parameter. If your Arduino NANO
has the 'ym2203_streamer.ino' uploaded, then the emulator and connected YM Bard should start
producing tunes from the arcade game.

Board production
------------------
To produce YM Bard you'll need some basic SMT component soldering skills. All components can be
soldered by hand with a modern soldering iron with controllable temperature (like TS100
or similar) set to 330 to 350 deg C. The list of components is located in 'bom_fm_bard_2203.csv'
file. The PCB can be fabricated by jlcpcb.com, pcbway.com, allpcb.com or other online services. 
Use the zip archive stored in the 'gerbers' directory and upload it to the manufacturer's web site
of your choice. Upload the 'ym2203_r2_fab.zip' archive and set the following parameters (if required). 
  
  * 2 layer board
  * PCB Thickness: 1.6, or 1.2 mm
  * Copper Weight: 1
  * The rest of the options can stay default or choose whatever you fancy (colors, finish etc.)

When populating components either start with the smallest SMT parts first (resistors and small capacitors)
or start with the SOIC-8 LM358 OpAmps so that you have plenty of room for the soldering iron tip.
After that, solder the bigger 10uF capacitors - check the capacitor's polarity. The SMT caps use
the stripe on the positive (+) side, the electrolytic TH caps use the stripe on the negative (-) side.
Solder the through hole parts and the audio connector at the end. 
I strongly recommend to use IC sockets for YM2203C chips and also for YM3014B chips.
IC sockets ensure you do not overheat and damage the chips while soldering. They will also allow
you to swap the chip in case you get faulty chips sourced from Aliexpress (not uncommon).

If you are not familiar with SMT soldering check some YT videos and use one of the YM Bard PCB 
as a practice board to solder some SMT resistors and caps. It is not that hard, just requires
good tweezers, use of soldering flux and a bit of practice.

Audio output
--------------
The output audio signal is routed through J1 'mixing' header. The header is supposed to be populated
by jumper caps that connect individual sound sources to either a stereo or mono output. The sound
sources are FM1, PSG1 (both coming from the YM2203 #1) and FM2 and PSG2 (both coming from YM2203 #2).
Yes, YM2203 chips have separate output for FM channels and for the PSG channels. The PSG channel
volume can be adjusted (relatively to the FM channels) by RV1 and RV2 potentiometers.
The middle jumper position on J1 - when shorted - joins all outputs to a mono signal (routed to
both Left and Right audio channel). By default the jumpers are supposed to be all connected (see the 
YM bard image), but you can undo them if you want to disable certain audio sources from the final output. 
Additionally/optionally you might want to design/add a post processing audio option to YM Bard 
by connecting it via J1 and J2 header (has GND and 5V pins). The output is not amplified but
is capable of driving small headphones via J5 headphone socket. If you want louder sound use
PC speakers with an amplifier connected to the headphone socket.
