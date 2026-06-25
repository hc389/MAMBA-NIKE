# Makefile for MAMBA-NIKE on NUCLEO (ARM Cortex-M4 / M7)
#
# Targets:
#   make              Build KAT_KEX.elf + KAT_KEX.bin
#   make flash        Flash to NUCLEO via ST-Link (requires st-flash)
#   make clean        Remove build artifacts
#
# Toolchain: arm-none-eabi-gcc (GNU Arm Embedded Toolchain)

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# Select your MCU:
#   STM32F446RE  (NUCLEO-F446RE):  -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
#   STM32F767ZI  (NUCLEO-F767ZI):  -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
#   STM32H743ZI  (NUCLEO-H743ZI):  -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS  = $(MCU_FLAGS) -Os -Wall -Wextra -fomit-frame-pointer \
          -ffunction-sections -fdata-sections \
          -DSTM32F446xx
LDFLAGS = -T STM32F446RE.ld -Wl,--gc-sections \
          --specs=nano.specs --specs=nosys.specs

ASFLAGS = $(MCU_FLAGS)

# NGCC KAT framework (unmodified except KAT_KEX_nucleo.c)
KAT_SRC = KAT_KEX_nucleo.c auxfunc.c drng.c

# Algorithm core
CORE_SRC = crypto_stream_chacha20.c poly.c toom.c \
           error_correction.c newhope.c reduce.c fips202.c \
           KEX_AlgorithmInstance.c

# Platform support
PLAT_SRC = syscalls.c startup_stm32f446xx.s

# All object files
OBJS = $(KAT_SRC:.c=.o) $(CORE_SRC:.c=.o) $(PLAT_SRC:.s=.o)
OBJS := $(OBJS:.c=.o)

all: KAT_KEX.elf KAT_KEX.bin

KAT_KEX.elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lm
	$(SIZE) $@

KAT_KEX.bin: KAT_KEX.elf
	$(OBJCOPY) -O binary $< $@

# Generic C compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assembly compilation
%.o: %.s
	$(CC) $(ASFLAGS) -c $< -o $@

flash: KAT_KEX.bin
	st-flash write KAT_KEX.bin 0x08000000

clean:
	rm -f KAT_KEX.elf KAT_KEX.bin *.o

.PHONY: all flash clean
