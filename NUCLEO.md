# MAMBA-NIKE 在 NUCLEO 平台测试指南

## 1. 平台兼容性分析

### 1.1 支持的 NUCLEO 型号

| 型号 | Flash | SRAM | 适用预设 |
|------|-------|------|---------|
| **NUCLEO-F446RE** (Cortex-M4) | 512 KB | 128 KB | NIKE-128 仅（需静态内存优化） |
| **NUCLEO-F767ZI** (Cortex-M7) | 2 MB | 512 KB | NIKE-128/192/256/384 |
| **NUCLEO-H743ZI** (Cortex-M7) | 2 MB | 1 MB | **全部预设**（推荐） |

### 1.2 内存需求估算

| 预设 | n | Poly 栈内存 | Toom 堆内存 | **总 RAM** | 说明 |
|------|---|------------|------------|---------|------|
| NIKE-128 | 1024 | ~16 KB | ~123 KB | **~139 KB** | F4 需优化，H7 直接可用 |
| NIKE-192 | 1024 | ~16 KB | ~123 KB | **~139 KB** | 同上 |
| NIKE-256 | 1024 | ~16 KB | ~123 KB | **~139 KB** | 同上 |
| NIKE-384 | 2048 | ~32 KB | ~246 KB | **~278 KB** | 需 F7/H7 |
| NIKE-512 | 4096 | ~64 KB | ~494 KB | **~558 KB** | 仅 H7 |

> **注意**：上述估算假设 `poly_convolution` 中 `int64_t aa[n]`、`bb[n]`、`prod[2n-1]` 使用 `malloc`。
> F4 系列需改为静态分配以省去堆开销，实际可缩减至 ~50 KB。

## 2. 环境准备

### 2.1 安装 ARM 工具链

```bash
# macOS
brew install arm-none-eabi-gcc

# Linux (Ubuntu/Debian)
sudo apt install gcc-arm-none-eabi

# 验证安装
arm-none-eabi-gcc --version
```

### 2.2 获取 STM32Cube HAL

```bash
git clone https://github.com/STMicroelectronics/STM32CubeF4.git
# 或
git clone https://github.com/STMicroelectronics/STM32CubeH7.git
```

### 2.3 项目目录结构

```
MAMBA-NIKE/
├── ref/                    # 原始参考代码
├── nucleo/                 # NUCLEO 移植代码（新建）
│   ├── Makefile
│   ├── linker_script.ld
│   ├── startup_stm32f4xx.s    # 启动文件
│   ├── syscalls.c             # 系统调用桩
│   ├── main.c                 # 测试主程序
│   ├── uart_printf.c/.h       # UART 重定向 printf
│   ├── randombytes_stm32.c/.h # STM32 硬件 RNG 适配
│   ├── <软链接到 ../ref/>     # poly.c, toom.c, newhope.c 等
│   └── params_nucleo.h        # 参数配置
└── TESTING.md
```

## 3. 代码修改

### 3.1 随机数生成器

替换 `/dev/urandom`（`ref/randombytes.c`）为 STM32 硬件 RNG：

```c
// nucleo/randombytes_stm32.c
#include "randombytes.h"
#include "stm32f4xx_hal.h"

// 使用 STM32 硬件真随机数发生器（RNG）
void randombytes(unsigned char *x, unsigned long long xlen)
{
    static RNG_HandleTypeDef hrng;
    static int initialized = 0;

    if (!initialized) {
        __HAL_RCC_RNG_CLK_ENABLE();
        hrng.Instance = RNG;
        HAL_RNG_Init(&hrng);
        initialized = 1;
    }

    unsigned long long i = 0;
    uint32_t rnd;
    while (i < xlen) {
        HAL_RNG_GenerateRandomNumber(&hrng, &rnd);
        for (int j = 0; j < 4 && i < xlen; j++, i++)
            x[i] = (rnd >> (8 * j)) & 0xFF;
    }
}
```

**备选方案**（无 RNG 的型号）：用 ChaCha20 从固定种子生成伪随机数：

```c
#include "crypto_stream_chacha20.h"

void randombytes(unsigned char *x, unsigned long long xlen)
{
    // 固定种子 — 仅用于功能测试，不可用于生产
    static unsigned char seed[32] = {0};
    static unsigned char nonce[8] = {0};
    static unsigned long long pos = 0;

    unsigned char buf[64];
    unsigned long long remaining = xlen;
    while (remaining > 0) {
        crypto_stream_chacha20(buf, 64, nonce, seed);
        unsigned long long take = remaining < 64 ? remaining : 64;
        for (unsigned long long j = 0; j < take; j++)
            x[xlen - remaining + j] = buf[(pos + j) & 63];
        remaining -= take;
        nonce[0]++;  // 递增 nonce
    }
}
```

### 3.2 malloc/free（仅 F4 需要）

NUCLEO-F4 系列 SRAM 仅 128 KB，`malloc` 从堆分配得不偿失。
改为**静态内存池**：在 `poly_convolution` 和 `toom4_mul` 中用全局静态数组替代 calloc/free。

```c
// nucleo/mempool.h
#define MEMPOOL_SIZE  96256   // 94 KB, 够 NIKE-128 Toom-Cook-4

extern uint8_t toom_mempool[MEMPOOL_SIZE];

static inline void *toom_alloc(size_t n) {
    static size_t offset = 0;
    void *ptr = &toom_mempool[offset];
    offset += n;
    return ptr;
}

static inline void toom_free_all(void) {
    // 不真正释放，重置偏移量即可
    // 在 poly_convolution 返回前调用
}
```

> H7 系列（≥ 512 KB SRAM）可直接使用 `malloc`/`free`。

### 3.3 printf 重定向到 UART

```c
// nucleo/uart_printf.c
#include <stdio.h>
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart2;  // NUCLEO 默认使用 USART2 (ST-Link VCP)

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

### 3.4 测试主程序

```c
// nucleo/main.c
#include "stm32f4xx_hal.h"
#include "../ref/newhope.h"
#include "../ref/randombytes.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    printf("\r\n=== MAMBA-NIKE on NUCLEO ===\r\n");
    printf("PARAM_N=%d PARAM_Q=%d PARAM_K=%d\r\n",
           PARAM_N, PARAM_Q, PARAM_K);

    poly sk_a;
    unsigned char key_a[32], key_b[32];
    unsigned char senda[NEWHOPE_SENDABYTES];
    unsigned char sendb[NEWHOPE_SENDBBYTES];

    int ok = 0, fail = 0;
    int N = 100;   // 嵌入式平台减少测试次数

    for (int i = 0; i < N; i++) {
        newhope_keygen(senda, &sk_a);
        newhope_sharedb(key_b, sendb, senda);
        newhope_shareda(key_a, &sk_a, sendb);

        if (!memcmp(key_a, key_b, 32)) ok++;
        else fail++;

        if ((i + 1) % 10 == 0)
            printf("  tested %d/%d: ok=%d fail=%d\r\n", i+1, N, ok, fail);
    }

    printf("Result: agree=%d fail=%d rate=%.2f%%\r\n",
           ok, fail, 100.0 * ok / N);

    // LED 闪烁指示结果
    if (fail == 0) {
        // 绿灯：全部通过
        while (1) {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);  // LED1 (green)
            HAL_Delay(500);
        }
    } else {
        // 红灯闪烁：有失败
        while (1) {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14); // LED3 (red)
            HAL_Delay(200);
        }
    }
}
```

## 4. Makefile（ARM 交叉编译）

```makefile
# nucleo/Makefile
CC      = arm-none-eabi-gcc
CFLAGS  = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
          -Os -Wall -Wextra -DSTM32F446xx -DUSE_HAL_DRIVER
LDFLAGS = -T linker_script.ld -Wl,--gc-sections -specs=nosys.specs \
          -specs=nano.specs -u _printf_float

# HAL 库路径（按实际位置修改）
HAL_DIR = ../STM32CubeF4/Drivers
CMSIS   = $(HAL_DIR)/CMSIS

INC = -I. -I$(HAL_DIR)/STM32F4xx_HAL_Driver/Inc \
      -I$(CMSIS)/Device/ST/STM32F4xx/Include \
      -I$(CMSIS)/Include \
      -I../ref

# 参考实现源文件（软链接）
REF_SRC = ../ref/poly.c ../ref/toom.c ../ref/error_correction.c \
          ../ref/newhope.c ../ref/reduce.c ../ref/fips202.c \
          ../ref/crypto_stream_chacha20.c

# 平台适配源文件
NUCLEO_SRC = main.c uart_printf.c randombytes_stm32.c mempool.c \
             startup_stm32f446xx.s syscalls.c

HAL_SRC = $(HAL_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
          $(HAL_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c \
          $(HAL_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
          $(HAL_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
          $(HAL_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
          $(HAL_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c

all: nucleo_test.elf nucleo_test.bin

nucleo_test.elf: $(NUCLEO_SRC) $(REF_SRC) $(HAL_SRC)
	$(CC) $(CFLAGS) $(INC) $^ $(LDFLAGS) -o $@

nucleo_test.bin: nucleo_test.elf
	arm-none-eabi-objcopy -O binary $< $@

flash: nucleo_test.bin
	st-flash write $< 0x08000000

clean:
	rm -f nucleo_test.elf nucleo_test.bin nucleo_test.map

.PHONY: all flash clean
```

## 5. 烧录和测试

### 5.1 使用 ST-Link（NUCLEO 板载）

```bash
# 安装 stlink 工具
# macOS: brew install stlink
# Linux:  sudo apt install stlink-tools

cd nucleo
make flash
```

### 5.2 使用 STM32CubeProgrammer

```bash
STM32_Programmer_CLI -c port=SWD -w nucleo_test.bin 0x08000000 -v
```

### 5.3 查看输出

```bash
# 串口终端（NUCLEO 虚拟串口 /dev/ttyACM0 或 /dev/cu.usbmodem*）
screen /dev/ttyACM0 115200
# 或
minicom -D /dev/ttyACM0 -b 115200
```

### 5.4 期望输出

```
=== MAMBA-NIKE on NUCLEO ===
PARAM_N=1024 PARAM_Q=65536 PARAM_K=2
  tested 10/100: ok=10 fail=0
  tested 20/100: ok=20 fail=0
  ...
  tested 100/100: ok=100 fail=0
Result: agree=100 fail=0 rate=100.00%
```

## 6. 各预设 F4 优化策略

### 6.1 NIKE-128/192/256（n=1024）

适合 NUCLEO-F446RE（128 KB SRAM），需以下优化：

1. **静态内存池替代 malloc**：~94 KB 静态数组
2. **栈多项式缩减**：将 `sharedb` 中的 8 个 `poly` 改为动态复用
3. **编译优化**：`-Os`（体积优化）+ `-flto`（链接时优化）

### 6.2 NIKE-384（n=2048）

需 NUCLEO-F767ZI 或 H7。额外建议：

1. 使用 DTCM（紧耦合数据存储器）存放 freq 访问的数组
2. 用 ITCM 存放热代码路径

### 6.3 NIKE-512（n=4096）

仅 NUCLEO-H743ZI。注意：

1. 单次 keygen + sharedb + shareda 约需 **5-15 秒**（取决于时钟频率）
2. 建议时钟设为 480 MHz（H7 最大频率）

## 7. 故障排查

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| `HardFault_Handler` | 栈溢出 | 增大 linker script 中栈大小（至少 8 KB） |
| `malloc` 返回 NULL | 堆不够 | F4：改用静态内存池；H7：增大 heap 尺寸 |
| 密钥一致率 < 95% | `randombytes` 有 bug | 先用固定种子 PRNG 验证算法正确性 |
| 编译不过（`-mcpu` 错误） | 工具链不对 | 确认 `arm-none-eabi-gcc` 路径正确 |
| UART 无输出 | 波特率/USART 配置错 | NUCLEO 默认 115200-8-N-1, USART2 |
| RNG 不工作 | RNG 时钟未使能 | 确认 `__HAL_RCC_RNG_CLK_ENABLE()` |

## 8. 链接器脚本要点

```ld
/* nucleo/linker_script.ld — 适用于 NUCLEO-F446RE */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM  (rwx): ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    .isr_vector : { ... } >FLASH
    .text       : { ... } >FLASH
    .rodata     : { ... } >FLASH
    .data       : { ... } >SRAM AT>FLASH
    .bss        : { ... } >SRAM
    .heap       : { ... } >SRAM    /* 给 malloc 预留 16 KB */
    .stack      : { ... } >SRAM    /* 栈 8 KB */
}
```

## 9. 快速开始（推荐路径）

**最简方案**：使用 **NUCLEO-H743ZI** + **NIKE-128** 预设。

1. 安装 `arm-none-eabi-gcc`
2. 克隆本仓库和 STM32CubeH7
3. 按第 3 节修改 `randombytes_stm32.c` 和 `uart_printf.c`
4. 复制第 4 节 Makefile（将 CFLAGS 改为 `-mcpu=cortex-m7`）
5. `make flash && screen /dev/ttyACM0 115200`
