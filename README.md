rtl-entropy
===========

An entropy generator using rtl-sdr.

The idea is to sample atmospheric noise at some starting frequency, run it through some quality tests, and if it passes muster, pass it on. If it's failing the quality tests, jump frequencies and try again.

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


Credits
-------
rtl_entropy was written by Paul Warren <pwarren@pwarren.id.au>
Uses code from:
 rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de> 
 and
 some ideas from [http://openfortress.org/cryptodoc/random/noise-filter.c] by Rick van Rein <rick@openfortress.nl>
