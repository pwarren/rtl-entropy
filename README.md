Abandoned
=========

This project is abandoned. Use a [OneRNG](https://onerng.info/) or a [ChaosKey](https://altusmetrum.org/ChaosKey/) instead.

This code will stay here and go read only.


rtl-entropy
===========

rtl-entropy is software using rtl-sdr to turn your DVB-T dongle into a high quality entropy source. It samples atmospheric noise, does Von-Neumann debiasing, runs it through the FIPS 140-2 tests, then optionally (`-e`) does Kaminsky debiasing if it passes the FIPS tests, then writes to the output. It can be run as a Daemon which by default writes to a FIFO, which can be read by rngd to add entropy to the system pool.

If you're serious about the cryptographic security of your entropy source, you should probably short, or put a 75 Ohm load on the antenna port, and put the whole assembly in a shielded box. Then you're getting entropy from the thermal noise of the amplifiers which is much harder to interfere with than atmospheric radio. 

Both of these are analog entropy sources.

This software has been tested on debian linux 7.1, but should work on any linux distribution, and might run on OS X and other POSIX compliant operating systems.


Links
-----
[lists](http://lists.rtl-entropy.org/)

[code](https://github.com/pwarren/rtl-entropy)

Dependencies
------------

* [rtl-sdr](http://sdr.osmocom.org/trac/wiki/rtl-sdr) (or librtlsdr-dev)
* libcap-dev
* openssl (libssl-dev)
* pkg-config

Note: If you want rtl-sdr to automatically detach the kernel driver, compile it with the cmake flag: `-DDETACH_KERNEL_DRIVER`. This is already done by most distribution's packaged librtlsdr.

eg:
```
cd ~/rtl-sdr
mkdir build
cd build
cmake ../ -DDETACH_KERNEL_DRIVER
```

then install as normal.

Installation
------------
```
git clone https://github.com/pwarren/rtl-entropy/
cd rtl-entropy
mkdir build
cd build 
cmake ../
make 
sudo make install
```

You can also do:
```
sudo make uninstall
```
Usage
-----
```
./rtl_entropy > high_entropy.bin
```
and press CTRL+C to stop, and do whatever you like with it.

or
```
./rtl_entropy -s 2.4M -f 101.5M -e | rngtest -c 1280 -p > high_entropy.bin
```
to set the sample rate to 2.4Msamples/s. the frequency to tune to as 101.5 MHz and do kaminsky debiasing (encryption), piped to rngtest which checks 1280 runs and stores it in high_entropy.bin


Please see the output of 
```
rtl_entropy -h
```
for further help!



For Daemon mode, along with rngd from rng-tools on linux:
```
rtl_entropy -b
rngd -r /var/run/rtl_entropy.fifo -W95%
```

The daemon mode by default uses `/var/run/rtl_entropy.fifo` for output and `/var/run/rtl_entropy.pid` for it's PID file. You may want to tweak the size of your entropy pool, and explore other options for rngd to maximize performance.

If you specify an output file with `-o`, rtl_entropy will open not attempt to create a FIFO, but will just open the file for writing.

To Do
-----

* Code Review
* Break things out of main()
* Website and Mailing Lists
* Look at Maurier tests
* make FIPS optional
* directly mix in to kernel as option with daemon mode

Done!
-----

* Implement daemon mode
* Need a mixing step xor fresh random bytes by with the old data
* add Kaminsky debiasing to my von neumann debiasing
* maybe a hash as well?
* Auto-detach kernel driver
* Further research and consultation with security experts is needed
* Do a release!
* Add FreeBSD flag and remove all libcap dependencies for FreeBSD
* fix licensing for OpenSSL

Credits
-------
Development supported by Star2Star communications.

rtl_entropy was written by Paul Warren <pwarren@pwarren.id.au> and has contributions from many github users, please see: https://github.com/pwarren/rtl-entropy/graphs/contributors

Uses code from:

  * rtl-sdr. Copyright (C) 2012 by Steve Markgraf http://sdr.osmocom.org/trac/wiki/rtl-sdr
  * rng-test-4. Copyright (C) 2001 Philipp Rumpf http://sourceforge.net/projects/gkernel/
  * http://openfortress.org/cryptodoc/random/noise-filter.c by Rick van Rein
  * snd-egd http://code.google.com/p/snd-egd/

Some helpful ideas from
  * Keenerd on the osmocom-sdr mailing list


Cryptography Discussion
-----------------------

As I mentioned above, capturing atmospheric radio is not terribly secure, if someone can transmit over the bandwidth you're listening on they could potentially influence the entropy your getting, and be able then to guess at your private keys!

To combat this, there's n option (-e) to do Kaminsky debiasing, which will dramatically alter the output if the input differs slightly. What this debiasing step does is take the bits rejected from the von neumann filter and create a SHA512 hash from those bits, then encrypt the entropy from the von Neumann filter using the hash as the key.

This makes the output much harder for an attacker to guess, entropy output will differ unpredictably for the attacker as they have to know the quantisation limits of your ADC to know which bits will be used where!

Happy for someone to correct me and justify why you should never use this for real entropy :)

Performance Testing
-------------------
```
rtl_entropy | rngtest -c 4096
```
on my core i5 1.8GHz Macbook Air 5,2 running debian 7.1

Least Significant bits: average bits/s from rngtest, failure count.
Averaged over 5 runs.  

2: 2577.507, 2

4: 4258.930, 5

6: 5157.910, 45

8: 20.330, 12,856


with 6 bits. 

CFLAGS: average bits/s

None: 3383.256

-O2: 5129.748

-O3: 5373.101

-march=native: 3873.854	

-march=native -O3: 5320.385
