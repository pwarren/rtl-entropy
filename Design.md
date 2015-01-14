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


I'm currently thinking of using threaded functions for each box, passing an input buffer and output buffer pointer, with lengths, and returning pass/fail.

The input functions will be async by default, as that's usually the fastest way to get data out from SDR devices.

transformation functions will probably need to work on blocks of some bytes, FIPS requires > 5000 bytes to work properly, and as some functions will change data sizes, they will also need to be asynchronous.

so the steps!

* input thread runs and sets up device and sets up async functions for data reception
* when an input thread has received some number of bytes, it passes its input into XOR
* XOR function, when it has received a block from all inputs, xors them together, and passes a blcok to either, the whitener, or the output.

* whitener receives a block, does its transform, and when it's accumulated a blocks worth of bytes, passes that to the next whitener or the output.

* Fips receives block, does its tests, on pass, outputs, on fail, drops block.

