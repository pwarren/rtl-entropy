CC=gcc
CFLAGS=-I. -O2 -Wall -Wextra
DEPS = fips.h util.h log.h
OBJ = rtl_entropy.o fips.o util.o log.o
LDFLAGS += -lm -lcap `pkg-config --libs librtlsdr`

%.o: %.c %(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

rtl_entropy: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o rtl_entropy
