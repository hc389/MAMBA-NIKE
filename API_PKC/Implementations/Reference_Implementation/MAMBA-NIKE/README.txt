MAMBA-NIKE Reference Implementation
==================================

Non-Interactive Key Exchange (NIKE) based on the NewHope protocol,
adapted to power-of-two modulus (q = 2^16 by default) with
Toom-Cook-4 polynomial multiplication.

This implementation provides the NGCC KEX API for MAMBA-NIKE
in portable C99 (no platform-specific intrinsics or assembly).

Build & Run
-----------
  make KAT_KEX        Build the KAT test-vector generator
  ./KAT_KEX           Run; output saved to output/KAT_KEX_MAMBA-NIKE.txt

The Makefile uses GCC (or compatible) with -O3 and requires only
standard libc. No external dependencies beyond the files in this folder.

Files
-----
  KEX_AlgorithmInstance.h     NGCC KEX API header (algorithm name, function decls)
  KEX_AlgorithmInstance.c     KEX implementation (bridge to newhope/poly/toom)
  KAT_KEX.c                   KAT test-vector generation framework (unmodified)
  drng.c / drng.h             Deterministic RNG (SM3-based, unmodified)
  auxfunc.c / auxfunc.h       Auxiliary output helpers (unmodified)

  params.h                    Scheme parameters (n, q, k, LOG2Q, byte sizes)
  poly.h / poly.c             Polynomial arithmetic (Toom-Cook-4 convolution)
  toom.h / toom.c             Toom-Cook-4 kernel (recursive, Bodrato sequence)
  newhope.h / newhope.c       Protocol: keygen, sharedb, shareda
  error_correction.h / .c     Reconciliation: helprec, rec
  reduce.h / reduce.c         Modular reduction (bitmask for power-of-2 q)
  fips202.h / .c              SHAKE128, SHA3-256 (Keccak)
  crypto_stream_chacha20.h/.c ChaCha20 stream cipher
  randombytes.h               Entropy interface (DRNG-backed in KAT mode)

Parameters (params.h)
---------------------
  PARAM_N  1024     Polynomial ring degree
  PARAM_K  2        Noise parameter (centered binomial Binomial(4)-2)
  PARAM_Q  65536    Modulus (2^16)
  LOG2Q    16       log2(q), used for shift-based division in reconciliation

Overriding via compiler flags: -DPARAM_N=2048 -DPARAM_Q=65536 -DLOG2Q=16

All five presets (NIKE-128/192/256/384/512) pass 100% key-agreement tests.
