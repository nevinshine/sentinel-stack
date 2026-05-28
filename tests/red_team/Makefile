CC ?= gcc
CFLAGS := -O2 -Wall -Werror -pthread

BINS := sentinel_strike sentinel_strike_ring0

.PHONY: all clean

all: $(BINS)

sentinel_strike: sentinel_strike.c
	$(CC) $(CFLAGS) -o $@ $^

sentinel_strike_ring0: sentinel_strike_ring0.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BINS)
