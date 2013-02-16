rtl-entropy
===========

An entropy generator using rtl-sdr.

The idea is to sample atmospheric noise at some starting frequency, run it through some quality tests, and if it passes muster, pass it on. If it's failing the quality tests, jump frequencies and try again. When the quality is good enough, add it to the system entropy pool via /dev/random. 

These features aren't implemented yet, currently it does some simplistic debiasing and outputs to STDOUT for testing via rngtest from rng-tools.

If you're serious about the cryptographic security of your entropy source, you should probably short the antenna, and put the whole assembly in a shielded box. Then you're getting entropy from the thermal noise of the amplifiers which is much harder to interfere with than atmospheric radio.

Dependencies
------------

* [rtlsdr](http://sdr.osmocom.org/trac/wiki/rtl-sdr)

Build
-----

* ./build 

or

* gcc rtl_entropy.c -o rtl_entropy -lm `pkg-config libs librtlsdr`

Usage
-----
apt-get install rng-tools

./rtl_entropy | rngtest -p > high_entropy.bin


To Do
-----
Next I'll look at using the rng-tools library to do the tests in the program, rather than piping to rngtest.

Then I'll look at how I can get the rng-tools entropy gathering daemon to accept data from us!


Notes from ##rtlsdr

Keenerd
 * Need a mixing step xor fresh random bytes by xoring with the old value
 * snd-egd has a good implementation
 * maybe a hash as well?
 * add Kaminsky debiasing to my von neumann debiasing

Credits
-------
rtl_entropy was written by Paul Warren <pwarren@pwarren.id.au>

Uses code from:

 rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de> 

 and

 some ideas from [http://openfortress.org/cryptodoc/random/noise-filter.c] by Rick van Rein <rick@openfortress.nl>
