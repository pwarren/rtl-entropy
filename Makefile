CC=gcc
CFLAGS=-I. -O3 -Wall -Wextra -pedantic
DEPS = fips.h util.h log.h
OBJ = rtl_entropy.o fips.o util.o log.o
LDFLAGS += -lm -lcap `pkg-config --libs librtlsdr` `pkg-config --libs openssl`

%.o: %.c %(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

rtl_entropy: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o rtl_entropy

install:
	install -s rtl_entropy /usr/local/sbin/