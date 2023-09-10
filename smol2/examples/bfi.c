#include <stdint.h>

#ifdef __SMOL__
#include <string.h>
#include <stdint.h>

typedef struct __attribute__((packed))
{
    uint8_t r, g, b;
} RgbColor;

typedef struct __attribute__((packed))
{
    char c;
    uint8_t col;
} FbEntry;

typedef struct __attribute__((packed))
{
    FbEntry chars[80*25];
    RgbColor palette[16];
    volatile char vsync_hit;
} Framebuffer;

Framebuffer *const FRAMEBUFFER_DEVICE = (Framebuffer*)(0xF0002000);

inline void write_char(char c)
{
    static int cur_idx = 0;

    FRAMEBUFFER_DEVICE->chars[cur_idx].c = c;

    ++cur_idx;
    if (cur_idx >= 80*25)
    {
        cur_idx = 0;
    }
}
#else
void write_char(char c);
#endif

#define DISPATCH() { ++ip; goto *jt[*ip]; }

void bfi(
    const char* restrict program,
    char* restrict tape
) {
    const char* ip = program;
    char* sp = tape;

    const static void** const jt[128] = {
        [0 ... 127] = &&dispatch,
        ['+'] = &&opInc,
        ['-'] = &&opDec,
        ['>'] = &&opR,
        ['<'] = &&opL,
        ['.'] = &&opWrite,
        ['['] = &&opLoopStart,
        [']'] = &&opLoopEnd
    };

    goto *jt[*ip];
    dispatch: __builtin_unreachable();
        while (*jt[*(++ip)] != &&dispatch) {}
        goto *jt[*ip];
    opInc:
        ++(*sp);
        DISPATCH();
    opDec:
        --(*sp);
        DISPATCH();
    opR:
        ++sp;
        DISPATCH();
    opL:
        --sp;
        DISPATCH();
    opWrite:
        write_char(*sp);
        DISPATCH();
    opLoopStart:
        if ((*sp) == 0) {
            int depth = 0;
            for (;;)
            {
                ++ip;
                if (*ip == '[')
                {
                    ++depth;
                }
                else if (*ip == ']')
                {
                    --depth;
                    if (depth == 0)
                    {
                        break;
                    }
                }
            }
            goto *jt[*ip];
        }
        DISPATCH();
    opLoopEnd:
        if ((*sp) != 0) {
            int depth = 0;
            for (;;)
            {
                --ip;
                if (*ip == ']')
                {
                    ++depth;
                }
                else if (*ip == '[')
                {
                    --depth;
                    if (depth == 0)
                    {
                        break;
                    }
                }
            }
            goto *jt[*ip];
        }
        DISPATCH();
}