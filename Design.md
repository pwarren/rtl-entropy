Design Overview
===============

---------|  
rtl-input|--\
---------|   \
              \|---|   |-----------|       |-----------------|
brf-input|-----|XOR|---|VN Whitener|-  -  -|Kaminsky Whitener|
              /|---|   |-----------|       |-----------------|
             /                                      |
Onerng-in|--/                                       |
                                                |----|  |------|
						|FIPS|--|Output|
					        |----|  |------|



So, multiple instances of the different types of input, with an easy way of adding new ones, xored together, optionally fed through whiteners and FIPS tests, and finally output.

On further thought, perhaps each input method needs its own whitener, as they will all be biased differently.

The input functions will be async by default, as that's usually the fastest way to get data out from SDR devices.

transformation functions will probably need to work on blocks of some bytes, FIPS requires > 5000 bytes to work properly, and as some functions will change data sizes, they will also need to be asynchronous.

So the steps!

* control thread sets up input threads, and data paths
* thread runs and sets up device, sets up async functions for data reception
* when an input thread has received some number of bytes, it passes its
  input into XOR
* XOR, when it has received a block from all inputs, xors them together
  and passes a block to either, the whitener, or the output.
* whitener receives a block, does its transform, and when it's accumulated
  a block's worth of bytes, passes that to the next whitener or the output.
* Fips receives block, does its tests, on pass, outputs, on fail, drops
  block.
* Output block writes the block to the specified output, stdout, file,
  FIFO, whatever.

Use OsmoSDR for input abstraction?
* Another dependancy
* not as portable?
* Maybe have rtl_entropy_simple which just takes first available rtl_sdr 
  device, whitens, emits to linux pool.
* Adds build configuration complexity. Hooray for #IFDEF I guess!


Input characterisations
* Statistical analysis of what rtl_sdr outputs
* Similarly for some bladerf reader process





Program Flow
============

* parse command line
** Device string, one per device (use OsmoSDR?)
** raw output
** crc32 whitening
** VN whitening
** Kaminsky Whitening
** AMLS Whitening
** Output into linux pool RNDADDENTROPY()
** Output into BSD/OSX pool?
** Output to stdout
** Output to file
* set up input threads
** As per devices strings
* set up processing threads with pipes from input threads
** LSB Sift: Take raw bits from input threads, emit only x most LSBs
** Apply whitening methods to input streams
** XOR all available streams (What to do if great difference in speed?)
* set up output threads with pipes from processing threads
** Emit a buffer's worth of bits when available

Really need a bitstream style abstraction for all this! Doable with a lot of buffers and buffer counters I suppose, probably already an implementation available.

