/*
 * syscalls.c — Minimal system call stubs for bare-metal ARM.
 *
 * Provides the symbols required by newlib-nano (used by arm-none-eabi-gcc).
 * _write is redirected to UART (USART2 on NUCLEO).
 * _sbrk allocates from a static heap.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* Static heap for malloc — 64 KB (adjust based on available SRAM) */
#define HEAP_SIZE 65536
static unsigned char heap[HEAP_SIZE];
static unsigned char *heap_end = heap + HEAP_SIZE;
static unsigned char *heap_ptr = heap;

/*
 * _sbrk — Increase program data space.
 * Used by malloc() in newlib.
 */
void *_sbrk(int incr)
{
    unsigned char *prev = heap_ptr;
    if (heap_ptr + incr > heap_end)
    {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_ptr += incr;
    return (void *)prev;
}

/*
 * _write — Write to a file descriptor.
 * fd == 1 (stdout) / fd == 2 (stderr) → UART semihosting or ITM.
 * On NUCLEO without UART init, falls back to semihosting.
 */
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    /* Semihosting: write to debugger console via BKPT */
    for (int i = 0; i < len; i++)
    {
        /* ITM stimulus port 0 (if debugger connected) */
        /* Fallback: breakpoint semihosting (works with openocd + gdb) */
        __asm__ volatile(
            "mov r0, #0x03\n"   /* SYS_WRITEC */
            "mov r1, %[c]\n"
            "bkpt #0xAB\n"
            :
            : [c] "r" (ptr[i])
            : "r0", "r1"
        );
    }
    return len;
}

/*
 * _read — Read from a file descriptor (not used by KAT).
 */
int _read(int fd, char *ptr, int len)
{
    (void)fd; (void)ptr;
    return len;
}

/*
 * _close — Close a file (not used by KAT).
 */
int _close(int fd)
{
    (void)fd;
    return 0;
}

/*
 * _lseek — Seek within a file (not used by KAT).
 */
off_t _lseek(int fd, off_t offset, int whence)
{
    (void)fd; (void)offset; (void)whence;
    return 0;
}

/*
 * _fstat — File status (not used by KAT).
 */
int _fstat(int fd, struct stat *buf)
{
    (void)fd;
    buf->st_mode = S_IFCHR;
    return 0;
}

/*
 * _isatty — Check if fd is a terminal.
 */
int _isatty(int fd)
{
    (void)fd;
    return 1;
}
