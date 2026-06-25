MAMBA-NIKE — NGCC API Implementations
=========================================

Non-Interactive Key Exchange submission for the Next-Generation
Commercial Cryptographic Algorithms Program (NGCC).

Directory Layout
----------------
  Test_Vector/
    KAT_KEX_MAMBA-NIKE.txt     Known Answer Test vectors (10 seeds, 10 SS)

  Implementations/
    Reference_Implementation/
      MAMBA-NIKE/               Portable C99 (gcc/clang, any platform)
    Optimized_Implementation/
      MAMBA-NIKE/               AVX2 ChaCha20 assembly + portable fallback
    Additional_Implementation/
      MAMBA-NIKE/               Bare-metal ARM Cortex-M (NUCLEO boards)

Quick Start
-----------
  # Reference (portable C)
  cd Implementations/Reference_Implementation/MAMBA-NIKE
  make KAT_KEX && ./KAT_KEX

  # Optimized (AVX2 auto-detect)
  cd Implementations/Optimized_Implementation/MAMBA-NIKE
  make KAT_KEX && ./KAT_KEX

  # Additional (NUCLEO, requires arm-none-eabi-gcc)
  cd Implementations/Additional_Implementation/MAMBA-NIKE
  make && make flash

All three implementations produce the SAME KAT output
(MD5: 7578b6fa0e5aa69721c3323fe72186aa for default NIKE-128 preset).

Algorithm Summary
-----------------
  Type:       Non-Interactive Key Exchange (1-pass)
  Ring:       Z_q[x]/(x^1024 + 1)
  Modulus:    q = 2^16 = 65536 (power of two)
  Noise:      Centered binomial Binomial(4)-2, range [-2, 2]
  Multiply:   Toom-Cook-4 (O(n^1.404))
  Reduction:  & 0xFFFF (single bitmask instruction)
  Public key: 2080 bytes
  Message:    2176 bytes
  Shared key: 32 bytes (SHA3-256)

  Security (lattice-estimator, MATZOV model):
    NIKE-128: 156.69 bit  (n=1024, q=2^16)
    NIKE-192: 221.61 bit  (n=1024, q=2^14)
    NIKE-256: 303.10 bit  (n=1024, q=2^13)
    NIKE-384: 425.23 bit  (n=2048, q=2^16)
    NIKE-512: 1162.60 bit (n=4096, q=2^13)

Files Common to All Implementations
------------------------------------
  KEX_AlgorithmInstance.h     NGCC KEX API header
  KEX_AlgorithmInstance.c     KEX bridge implementation
  KAT_KEX*.c                  KAT test-vector generator
  drng.c / drng.h             Deterministic RNG (SM3-based)
  auxfunc.c / auxfunc.h       Auxiliary output functions
  params.h                    Scheme parameters
  poly.h / poly.c             Polynomial arithmetic
  toom.h / toom.c             Toom-Cook-4 kernel
  newhope.h / newhope.c       Protocol implementation
  error_correction.h / .c     Reconciliation
  reduce.h / reduce.c         Modular reduction
  fips202.h / .c              SHAKE128 / SHA3-256
  crypto_stream_chacha20.h/.c ChaCha20 stream cipher
  randombytes.h               Entropy interface

  These files MUST NOT be modified:
    drng.c, drng.h, auxfunc.c, auxfunc.h, KAT_KEX.c (framework version)
