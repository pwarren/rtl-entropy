rtl-entropy
===========

An entropy generator using rtl-sdr.

The idea is to sample atmospheric noise at some starting frequency, run it through some 
quality tests, and if it passes muster, pass it on to ther kernel, perhaps directly or 
via rng-tool or similar.



