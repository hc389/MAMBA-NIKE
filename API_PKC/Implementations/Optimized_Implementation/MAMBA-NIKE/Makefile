# Makefile for MAMBA-NIKE KAT KEX — Optimized Implementation
#
# Two build modes:
#   make           — auto-detect AVX2, falls back to portable
#   make AVX2=1    — force AVX2 on (requires x86_64 + AVX2 CPU)
#   make AVX2=0    — portable C only (ARM, generic x86_64, etc.)

CC = /usr/bin/gcc
AR = ar

# --- Choose AVX2 or portable ---
AVX2 ?= $(shell $(CC) -mavx2 -dM -E - < /dev/null 2>/dev/null | grep -q __AVX2__ && echo 1 || echo 0)

ifeq ($(AVX2),1)
  CFLAGS   = -Wall -Wextra -O3 -fomit-frame-pointer -mavx2 -march=native
  ASFLAGS  = -mavx2
  CHACHA_SRC = crypto_stream_chacha20.c
  CHACHA_OBJ = chacha.o
else
  CFLAGS   = -Wall -Wextra -O3 -fomit-frame-pointer
  CHACHA_SRC = crypto_stream_chacha20.c
  CHACHA_OBJ =
endif

# --- Algorithm source files ---
ALGO_SRC = KEX_AlgorithmInstance.c
CORE_OBJ = poly.o toom.o error_correction.o newhope.o reduce.o fips202.o \
           crypto_stream_chacha20.o

# --- KAT framework files ---
KAT_SRC  = KAT_KEX.c auxfunc.c drng.c

all: KAT_KEX

KAT_KEX: $(CORE_OBJ) $(CHACHA_OBJ) $(ALGO_SRC) $(KAT_SRC)
	$(CC) $(CFLAGS) -o KAT_KEX $(ALGO_SRC) $(CORE_OBJ) $(CHACHA_OBJ) $(KAT_SRC) -lm

# --- Core object rules ---
crypto_stream_chacha20.o: crypto_stream_chacha20.c crypto_stream_chacha20.h
	$(CC) $(CFLAGS) -c crypto_stream_chacha20.c

poly.o: poly.c poly.h params.h reduce.h toom.h
	$(CC) $(CFLAGS) -c poly.c

toom.o: toom.c toom.h params.h
	$(CC) $(CFLAGS) -c toom.c

error_correction.o: error_correction.c error_correction.h params.h poly.h
	$(CC) $(CFLAGS) -c error_correction.c

newhope.o: newhope.c newhope.h poly.h error_correction.h fips202.h
	$(CC) $(CFLAGS) -c newhope.c

reduce.o: reduce.c reduce.h params.h
	$(CC) $(CFLAGS) -c reduce.c

fips202.o: fips202.c fips202.h
	$(CC) $(CFLAGS) -c fips202.c

# --- Assembly rule (AVX2 only) ---
chacha.o: chacha.S
	$(CC) $(ASFLAGS) -c chacha.S -o chacha.o

run: KAT_KEX
	./KAT_KEX

clean:
	rm -f KAT_KEX *.o output/KAT_KEX_MAMBA-NIKE.txt

.PHONY: all run clean
