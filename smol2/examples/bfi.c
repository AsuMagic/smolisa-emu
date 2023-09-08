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

void write_char(char c)
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

#define DISPATCH() { ++ip; goto *jt[program[ip]]; }

void bfi(
    const char* restrict program,
    char* restrict tape
) {
    int ip = 0;
    int sp = 0;

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

    goto *jt[program[ip]];
    dispatch:
        goto *jt[program[++ip]];
    opInc:
        ++tape[sp];
        DISPATCH();
    opDec:
        --tape[sp];
        DISPATCH();
    opR:
        ++sp;
        DISPATCH();
    opL:
        --sp;
        DISPATCH();
    opWrite:
        write_char(tape[sp]);
        DISPATCH();
    opLoopStart:
        if (tape[sp] == 0) {
            int depth = 0;
            for (;;)
            {
                ++ip;
                if (program[ip] == '[')
                {
                    ++depth;
                }
                else if (program[ip] == ']')
                {
                    --depth;
                    if (depth == 0)
                    {
                        break;
                    }
                }
            }
            goto *jt[program[ip]];
        }
        DISPATCH();
    opLoopEnd:
        if (tape[sp] != 0) {
            int depth = 0;
            for (;;)
            {
                --ip;
                if (program[ip] == ']')
                {
                    ++depth;
                }
                else if (program[ip] == '[')
                {
                    --depth;
                    if (depth == 0)
                    {
                        break;
                    }
                }
            }
            goto *jt[program[ip]];
        }
        DISPATCH();
}