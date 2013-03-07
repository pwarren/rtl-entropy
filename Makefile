CC=gcc
CFLAGS=-I. -O2 -Wall -Wextra
DEPS = fips.h
OBJ = rtl_entropy.o fips.o
LDFLAGS += -lm `pkg-config --libs librtlsdr`

%.o: %.c %(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

rtl_entropy: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o rtl_entropy
