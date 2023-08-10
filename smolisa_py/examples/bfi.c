#include <stdint.h>

void write_char(char c);

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