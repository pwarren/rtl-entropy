rtl-entropy
===========

An entropy generator using rtl-sdr.

The idea is to sample atmospheric noise at some starting frequency, run it through some 
quality tests, and if it passes muster, pass it on to ther kernel, perhaps directly or 
via rng-tool or similar.

Dependencies
------------

* [rtlsdr](http://sdr.osmocom.org/trac/wiki/rtl-sdr)

Build
-----

* ./build 
or

* gcc rtl-entropy.c -o rtl-entropy -lm `pkg-config libs librtlsdr`

Usage
-----

Currently, run it piped to a FIFO which you set the RNG daemon to read from


Credits
-------

Rtlizer was written by Paul Warren.
Uses code from rtl_test by Steve Markgraf.
