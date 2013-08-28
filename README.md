rtl-entropy
===========

An entropy generator using rtl-sdr.

The idea is to sample atmospheric noise, run it throught the FIPS 140-2 tests, if it passes write it out to the specified output. 

If you're serious about the cryptographic security of your entropy source, you should probably short the antenna, and put the whole assembly in a shielded box. Then you're getting entropy from the thermal noise of the amplifiers which is much harder to interfere with than atmospheric radio.

Neither of these are 'Quantum Random' sources

Dependencies
------------

* [rtlsdr](http://sdr.osmocom.org/trac/wiki/rtl-sdr)

Build
-----

make 

Assumes you've got the rtl-sdr libraries and gcc installed.

Usage
-----

./rtl_entropy > high_entropy.bin
and press CTRL+C to stop, and do whatever you like with it.

or

./rtl_entropy -s 2.4M -f 101.5M | rngtest -c 1280 -p > high_entropy.bin

to set the sample rate to 2.4Msamples/s and the frequency to tune to as 101.5 MHz, piped to rngtest which checks 1280 runs and stores it in high_entropy.bin

You should be able to use rndaddentropy from [twuwand](http://github.com/rfinnie/twuewand)

./rtl_entropy -s 2.4M -f 101.5M | rndaddentropy


To Do
-----

- Further research and consultation with security experts is needed on:
 * Need a mixing step xor fresh random bytes by with the old data
 * maybe a hash as well?
 * add Kaminsky debiasing to my von neumann debiasing

- Code Review

Thanks to Keenerd on the osmocom-sdr mailing list.


Done!
-----

* Implement daemon mode that adds entropy to the system pool.


Credits
-------

rtl_entropy was written by Paul Warren <pwarren@pwarren.id.au>

Uses code from:

  * rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de> http://sdr.osmocom.org/trac/wiki/rtl-sdr

  * rng-test-4. Copyright (C) 2001 Philipp Rumpf http://sourceforge.net/projects/gkernel/

  * http://openfortress.org/cryptodoc/random/noise-filter.c by Rick van Rein <rick@openfortress.nl>

  * snd-egd http://code.google.com/p/snd-egd/