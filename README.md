rtl-entropy
===========

An entropy generator using rtl-sdr.

The idea is to sample atmospheric noise at some starting frequency, run it through some quality tests, and if it passes muster, pass it on. If it's failing the quality tests, jump frequencies and try again. When the quality is good enough, add it to the system entropy pool via /dev/random. 

Not all of this is implemented yet! Currently, it does the FIPS tests internally, and prints to stdout the bits that passed.

If you're serious about the cryptographic security of your entropy source, you should probably short the antenna, and put the whole assembly in a shielded box. Then you're getting entropy from the thermal noise of the amplifiers which is much harder to interfere with than atmospheric radio.

Dependencies
------------

* [rtlsdr](http://sdr.osmocom.org/trac/wiki/rtl-sdr)

Build
-----

make 

I've not got in to CMake yet, so please send me a pull request with a cmake implementation!

Usage
-----
apt-get install rng-tools

./rtl_entropy > high_entropy.bin
Then use the entropy you've collected however you like!

./rtl_entropy -s 2.4M -f 101.5M | rngtest -c 512 -p > high_entropy.bin
Set the sample rate to 2.5 Mega Samples/s and the listening frequency to 101.5MHz (a station where I live, slows down entropy collection!)

To Do
-----

* Implement daemon mode that adds entropy to the system pool.

From Keenerd on the osmocom-sdr mailing list.
 * Need a mixing step xor fresh random bytes by with the old data
 * snd-egd has a good implementation
 * maybe a hash as well?
 * add Kaminsky debiasing to my von neumann debiasing

Credits
-------
rtl_entropy was written by Paul Warren <pwarren@pwarren.id.au>

Uses code from:

  * rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de> http://sdr.osmocom.org/trac/wiki/rtl-sdr

  * rng-test-4. Copyright (C) 2001 Philipp Rumpf http://sourceforge.net/projects/gkernel/

  * http://openfortress.org/cryptodoc/random/noise-filter.c by Rick van Rein <rick@openfortress.nl>
