CC=gcc
CFLAGS=-I. -O3 -Wall -Wextra -pedantic `pkg-config --cflags librtlsdr`
DEPS = fips.h util.h log.h
OBJ = rtl_entropy.o fips.o util.o log.o

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LDFLAGS = -lm -lcap `pkg-config --libs librtlsdr` `pkg-config --libs libssl`
endif
ifeq ($(UNAME_S),Darwin)
	LDFLAGS = -lm `pkg-config --libs librtlsdr` `pkg-config --libs libssl`
endif

%.o: %.c %(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

rtl_entropy: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o rtl_entropy

install:
	install -s rtl_entropy /usr/local/sbin/
