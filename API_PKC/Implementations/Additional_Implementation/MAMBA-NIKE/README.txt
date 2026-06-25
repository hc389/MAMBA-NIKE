MAMBA-NIKE — NUCLEO Embedded Implementation
=============================================

Non-Interactive Key Exchange (NIKE) for STM32 NUCLEO development boards.

This implementation targets bare-metal ARM Cortex-M microcontrollers
and produces a KAT test-vector binary that outputs results over UART
or semihosting debug console.

Supported boards
----------------
  NUCLEO-F446RE  (Cortex-M4, 128 KB SRAM) — NIKE-128 only
  NUCLEO-F767ZI  (Cortex-M7, 512 KB SRAM) — NIKE-128/192/256/384
  NUCLEO-H743ZI  (Cortex-M7,   1 MB SRAM) — all presets

Prerequisites
-------------
  arm-none-eabi-gcc      GNU Arm Embedded Toolchain
  st-flash               ST-Link flashing utility (optional)

  macOS:   brew install arm-none-eabi-gcc stlink
  Ubuntu:  sudo apt install gcc-arm-none-eabi stlink-tools

Build
-----
  Edit the Makefile and set MCU_FLAGS for your board:
    NUCLEO-F446RE:  -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    NUCLEO-F767ZI:  -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard

  make                 Build KAT_KEX.elf and KAT_KEX.bin
  make flash           Flash to NUCLEO via ST-Link (st-flash)
  make clean           Remove build artifacts

Flash & Run
-----------
  make flash
  # Connect UART (USART2 on NUCLEO, 115200-8N1 via ST-Link VCP):
  screen /dev/ttyACM0 115200

Output (printed over UART)
--------------------------
  === KAT_KEX_MAMBA-NIKE ===
  Count = 0
  Seed_Len = 64
  Seed = 927F06B5...
  ...
  SS_Len = 32
  SS = 8B910B17...
  ...
  === KAT_KEX_MAMBA-NIKE COMPLETE (10/10) ===

Memory considerations
---------------------
  NIKE-128 (n=1024) requires ~140 KB RAM.
  F4 boards have 128 KB — use careful static allocation or reduce n.

Files
-----
  KEX_AlgorithmInstance.h/c   NGCC KEX API implementation
  KAT_KEX_nucleo.c            KAT generator (UART output, no file I/O)
  drng.c/h                    Deterministic RNG (SM3-based, unmodified)
  auxfunc.c/h                 Auxiliary output helpers (unmodified)
  syscalls.c                  Bare-metal newlib stubs (_sbrk, _write, etc.)
  startup_stm32f446xx.s       Cortex-M4 startup (vector table, .data/.bss init)
  STM32F446RE.ld              Linker script (FLASH 512K / SRAM 128K)

  params.h                    Scheme parameters
  poly.h/c                    Polynomial arithmetic (Toom-Cook-4)
  toom.h/c                    Toom-Cook-4 kernel
  newhope.h/c                 Protocol: keygen, sharedb, shareda
  error_correction.h/c        Reconciliation: helprec, rec
  reduce.h/c                  Modular reduction (bitmask)
  fips202.h/c                 SHAKE128, SHA3-256
  crypto_stream_chacha20.h/c  ChaCha20 stream cipher
  randombytes.h               Entropy interface (DRNG-backed)

Differences from Reference Implementation
-----------------------------------------
  - KAT_KEX_nucleo.c outputs to UART instead of files
  - syscalls.c provides bare-metal _sbrk, _write (semihosting)
  - startup + linker script for Cortex-M
  - arm-none-eabi-gcc toolchain instead of host gcc
  - Algorithm core source files are identical to Reference
