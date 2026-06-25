MAMBA-NIKE Optimized Implementation
===================================

Non-Interactive Key Exchange (NIKE) — AVX2-accelerated build.

Identical protocol logic to the Reference implementation, with an
AVX2 assembly implementation of ChaCha20 (chacha.S) that accelerates
noise sampling (~3x speedup on x86_64 with AVX2 support).

On non-AVX2 platforms the same code compiles as portable C (the
Reference ChaCha20 falls back automatically).

Build & Run
-----------
  make KAT_KEX        Auto-detects AVX2; falls back to portable C if unavailable
  make AVX2=1 KAT_KEX Force AVX2 (requires x86_64 + AVX2 CPU + GCC/Clang)
  make AVX2=0 KAT_KEX Force portable C (ARM / generic x86_64)
  ./KAT_KEX           Run; output saved to output/KAT_KEX_MAMBA-NIKE.txt

The KAT output is bit-identical to the Reference implementation
(MD5: 7578b6fa0e5aa69721c3323fe72186aa for NIKE-128 default).

Files (additional to Reference)
-------------------------------
  chacha.S                    AVX2 assembly ChaCha20 (chacha_avx2 entry point)
  crypto_stream_chacha20.c    Dual-path: __AVX2__ → chacha.S, else portable C

All other files are identical to the Reference implementation.
