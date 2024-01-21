
#include "serial.h"
#include "string.h"
#include "assert.h"

extern "C"
{

    extern Serial debug_serial;

#ifdef __ARMCC_VERSION
/*
keil armcc compiler, is not free, but much better in optimization(both speed and size),
on the other hand, keil is intended for mcu while gcc is not mainly for mcu
*/
#if (__ARMCC_VERSION >= 6010050) /* when use keil ac6 compiler */
    __asm(".global __use_no_semihosting\n\t");
    __asm(".global __ARM_use_no_argv \n\t");

    char *_sys_command_string(char *cmd, int len)
    {
        return NULL;
    }

#else /* when use keil ac5 compiler */

#pragma import(__use_no_semihosting)
    struct __FILE
    {
        int handle;
        /* Whatever you require here. If the only file you are using is */
        /* standard output using printf() for debugging, no file handling */
        /* is required. */
    };

#endif

#include "stdio.h"
    int _ttywrch(int ch)
    {
        ch = ch;
        return ch;
    }

    void _sys_exit(int x)
    {
        x = x;
    }

    FILE __stdout;

    int fputc(int ch, FILE *f)
    {
        static uint8_t is_out_ready = 0;
        if (is_out_ready == 0)
        {
            is_out_ready = 1;
            if (0 != debug_serial.open(DEV_ID_USART2, 256000))
                assert(false);
        }
        static char buf;
        buf = ch;
        debug_serial.send((uint8_t *)&buf, 1, 1);
        return ch;
    }
#elif defined(__GNUC__)
#include "stdio.h"
#include "unistd.h"
#include <sys/types.h>
#include <sys/stat.h>

    // refer to https://blog.csdn.net/CooCox_UP_Team/article/details/8465143

    extern int _end; // the heap region, defined in ld script

    caddr_t _sbrk(int incr)
    {
        static unsigned char *heap = NULL;
        unsigned char *prev_heap;
        if (heap == NULL)
        {
            heap = (unsigned char *)&_end;
        }
        prev_heap = heap;
        heap += incr;
        return (caddr_t)prev_heap;
    }

    int _close(int file)
    {
        return 0;
    }
    int _fstat(int file, struct stat *st)
    {
        st->st_mode = S_IFCHR;  // char device
        return 0;
    }
    int _isatty(int file)
    {
        return 1;   // terminal device
    }
    int _lseek(int file, int ptr, int dir)
    {
        return 0;
    }
    int _read(int file, char *ptr, int len)
    {
        return debug_serial.recv((uint8_t*)ptr, len);
    }
    int _write(int file, char *ptr, int len)
    {
        debug_serial.send((uint8_t *)ptr, len, 1);
        return len;
    }
    void abort(void)
    {
        /* Abort called */
        while (1)
            ;
    }
#endif
}
