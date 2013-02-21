CC=gcc
CFLAGS=-I. -O2 -lm
DEPS = fips.h
OBJ = rtl_entropy.o fips.o
CFLAGS += `pkg-config --libs librtlsdr`

%.o: %.c %(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

rtl_entropy: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm -f *.o rtl_entropy
