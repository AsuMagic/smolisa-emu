# smol2

## Memory

### Word size

CPU words are 32 bits, including memory addresses, ALU operands, and registers.

### Memory access

#### Granularity

Memory access instructions usually support a granularity of 8, 16 and 32 bits.

#### Endianness

Memory accesses are little-endian.  
Instructions are also encoded as little-endian.

#### Overlapping writes and instruction memory

Writes to memory that the instruction pointer may reach may not be reflected immediately.  
Ways to mitigate this are currently implementation-defined.

#### Unmapped memory

RAM address ranges are implementation defined.  
Out-of-range memory accesses are undefined behavior.

#### Alignment requirements

16-bit and 32-bit memory accesses must be aligned. Instructions should be aligned to 16-bit boundaries.

## Registers

Whether *all* registers are 0-initialized on reset is implementation-defined.
Certain registers are *always* 0-initialized on reset: `rip`.

The instruction pointer initializes in an implementation-defined manner (typically `0x0`).
TODO: this should not really be the case, memory mapping will make this make more sense

| Mnemonic   | Encoding | Use or ABI meaning                            | Saved    |
|------------|----------|-----------------------------------------------|----------|
|            |          | **General purpose registers**                 |          |
| `r0`       | `0x0`    | Fast target for certain encodings             | Caller   |
| `r1`       | `0x1`    | ABI: Arg #0 or vararg count, return register  | Caller   |
| `r2`       | `0x2`    | ABI: Arg #1                                   | Caller   |
| `r3`       | `0x3`    | ABI: Arg #2                                   | Caller   |
| `r4`       | `0x4`    | ABI: Arg #3                                   | Caller   |
| `r5`       | `0x5`    | ABI: Arg #4                                   | Caller   |
| `r6`       | `0x6`    | ABI: Arg #5                                   | Caller   |
| `r7`       | `0x7`    |                                               | Callee   |
| `r8`       | `0x8`    |                                               | Callee   |
| `r9`       | `0x9`    |                                               | Callee   |
| `r10`      | `0xA`    |                                               | Callee   |
| `r11`      | `0xB`    |                                               | Callee   |
| `r12`      | `0xC`    |                                               | Callee   |
|            |          | **Specific-purpose registers**                |          |
| `rpl`      | `0xD`    | Literal pool start address                    | Callee   |
| `rpg`      | `0xE`    | Global pool start address                     | Callee   | 
| `rps`      | `0xF`    | Stack pointer                                 | Callee   |

Certain special registers cannot be addressed directly, but may be manipulated
and used by certain instructions.

| Name       | Width  | Meaning             |
|------------|--------|---------------------|
| `T`        | 1 bit  | **T**est bit        |
| `rip`      | 32-bit | Instruction pointer |

#### Stack

The standard way to use the stack is to initialize it to the *highest* address. The stack grows *downwards* in the address space.

For instance, `push(r5)` is roughly equivalent to:

```python
    addi(rps, -4),
    s32(rps, r5)
```

... and `pop(r5)` is roughly equivalent to:

```python
    lu32(rps, r5),
    add_i(rps, 4)
```

#### The stack and interrupts

The stack should be set up in a consistent state before interrupts (TODO) are enabled.

When an interrupt fires, the CPU will retire all instructions in the pipeline.  
After this, registers will be dumped to the stack. As such, memory below the
stack pointer should be expected to be mutated at any time.

## Branching

No delay slots are used for branching instructions.  
Thus, the instruction executed immediately after a successful branch will be the
targeted IP.

Conditional branches and loads read the special `T` bit register.
The `T` bit is manipulated by certain arithmetic and test instructions.

## Instructions

TODO:

- sel bytes (kind of like PEXT but at byte level) - would allow doing zext
- check more of what SH might have interesting
- check more of what RV might have interesting
- check more of what Thumb might have interesting

|Format | Instr[0:7] | Mnemonic                      | Description                                           | Pseudocode                                             |
|-------|------------|-------------------------------|-------------------------------------------------------|--------------------------------------------------------|
|       |            |                               | _**Loads**_                                           |                                                        |
| R4W4  | `00000000` | `l8(addr:R4, dst:R4)`         | **L**oad u**8** from memory (zext)                    | `dst <- u32(mem8(addr))`                               |
| R4W4  | `00000001` | `l16(addr:R4, dst:R4)`        | **L**oad u**16** from memory (zext)                   | `dst <- u32(mem16(addr))`                              |
| R4W4  | `00000010` | `l32(addr:R4, dst:R4)`        | **L**oad u**32** from memory                          | `dst <- mem32(addr)`                                   |
| R4-4  | `00000011` | `pop(src:R4)`                 | **Pop** from stack (rps)                              | `dst <- mem32(rps); rps <- rps + 4`                    |
| R2I8  | `000001--` | `lo8_r0(base:R2, imm:I8)`     | **L**oad **o**ffset u**8** from memory to **R0**      | `r0 <- u32(mem8(base + u32(imm)))`                     |
| R2Id8 | `000010--` | `lo16_r0(base:R2, imm:Id8)`   | **L**oad **o**ffset u**16** from memory to **R0**     | `r0 <- u32(mem16(base + u32(imm) << 1))`               |
| R2Iq8 | `000011--` | `lo32_r0(base:R2, imm:Iq8)`   | **L**oad **o**ffset u**32** from memory to **R0**     | `r0 <- mem32(base + u32(imm) << 2))`                   |
| W4I8  | `0001----` | `lsi(imm:I8, dst:R4)`         | **L**oad from **s**8 **i**mmediate                    | `dst <- s32(imm)`                                      |
| Id12  | `0010----` | `liprel_r0(imm:Id12)`         | **L**oad **ip-rel**ative ±8K to **r0**                | `r0 <- rip + 2 + s32(dst << 1)`                        |
|       |            |                               | _**Stores**_                                          |                                                        |
| R4R4  | `00110000` | `s8(addr:R4, src:R4)`         | **S**tore u**8** to memory                            | `mem8(addr) <- src[:7]`                                |
| R4R4  | `00110001` | `s16(addr:R4, src:R4)`        | **S**tore u**16** to memory                           | `mem16(addr) <- src[:15]`                              |
| R4R4  | `00110010` | `s32(addr:R4, src:R4)`        | **S**tore u**32** to memory                           | `mem32(addr) <- src`                                   |
| R4-4  | `00110011` | `push(src:R4)`                | **Push** to rps                                       | `rps <- rps - 4; mem32(rps) <- src`                    |
| R2I8  | `001101--` | `so8_r0(base:R2, imm:I8)`     | **S**tore **o**ffset u**8** to memory from **R0**     | `mem8(addr + u32(imm)) <- src[:7]`                     |
| R2Id8 | `001110--` | `so16_r0(base:R2, imm:Id8)`   | **S**tore **o**ffset u**16** to memory from **R0**    | `mem16(addr + u32(imm) << 1)) <- src[:15]`             |
| R2Iq8 | `001111--` | `so32_r0(base:R2, imm:Iq8)`   | **S**tore **o**ffset u**32** to memory from **R0**    | `mem32(addr + u32(imm) << 2)) <- src`                  |
|       |            |                               | _**Pool loads**_                                      |                                                        |
| W4I8  | `0100----` | `pl_l32(dst:R4, imm:Iq8)`     | **rpl** **L**oad u**32** from memory ±1K              | `dst <- mem32(rpg + (u32(imm) << 2))`                  |
| I12   | `0101----` | `pg_l32_r0(imm:Iq12)`         | **rpg** **L**oad u**32** from memory ±16K to **R0**   | `R0 <- mem32(rpg + (u32(imm) << 2))`                   |
|       |            |                               | _**Pool stores**_                                     |                                                        |
| I12   | `0110----` | `p0_su32_r0(imm:Iq12)`        | **rpg** **S**tore **u32** to memory ±16K from **R0**  | `mem32(rpg + (imm << 2)) <- r0`                        |
|       |            |                               | _**Pool IP-relative init**_                           |                                                        |
| I12   | `0111----` | `liprel_pl(imm:Io12)`         | **L**oad **ip-rel**ative ±32K to **rpl**              | `rpl <- rip + (imm << 3)`                              |
|       |            |                               | _**Tests and `T`-bit manipulation**_                  |                                                        |
| R4R4  | `10000000` | `tltu(a:R4, b:R4)`            | **T**est if **l**ower **t**han (**u**nsigned)         | `T <- (a < b)`                                         |
| R4R4  | `10000001` | `tlts(a:R4, b:R4)`            | **T**est if **l**ower **t**han (**s**igned)           | `T <- (s32(a) < s32(b))`                               |
| R4R4  | `10000010` | `tgeu(a:R4, b:R4)`            | **T**est if **g**reater or **e**qual (**u**nsigned)   | `T <- (a >= b)`                                        |
| R4R4  | `10000011` | `tges(a:R4, b:R4)`            | **T**est if **g**reater or **e**qual (**s**igned)     | `T <- (s32(a) >= s32(b))`                              |
| R4R4  | `10000100` | `te(a:R4, b:R4)`              | **T**est if **e**qual to                              | `T <- (a == b)`                                        |
| R4R4  | `10000101` | `tne(a:R4, b:R4)`             | **T**est if **n**ot **e**qual to                      | `T <- (a != b)`                                        |
| R4I4  | `10000110` | `tltsi(a:R4, b:I4)`           | **T**est if **l**ower **t**han **s**igned **i**mm.    | `T <- (s32(a) < s32(b))`                               |
| R4I4  | `10000111` | `tgesi(a:R4, b:I4)`           | **T**est if **g**reater or **e**qual (**s**igned)     | `T <- (s32(a) >= s32(b))`                              |
| R4I4  | `10001000` | `tei(a:R4, b:I4)`             | **T**est if **e**qual to **i**mmediate                | `T <- (a == b)`                                        |
| R4I4  | `10001001` | `tnei(a:R4, b:I4)`            | **T**est if **n**ot **e**qual to **i**mmediate        | `T <- (a != b)`                                        |
|       |            |                               | _**Branching and conditional ops**_                   |                                                        |
| R4-4  | `10001010` | `j(addr:R4)`                  | **J**ump unconditionally                              | `rip <- addr`                                          |
| R4-4  | `10001011` | `jct(index:R4)`               | **J**ump into **c**ode **t**able                      | `rip <- mem32(rip + 2 + s32(dst << 1))`                |
| R4R4  | `10001100` | `jdt(index:R4, addr:R4)`      | **J**ump into **d**ata **t**able                      | `rip <- mem32(addr + s32(dst << 1))`                   |
| | | |yolo: those jump insns may suck; unsure how much latency they force in the pipeline but it doesnt seem pretty |
| R4-4  | `10001101` | `c_j(addr:R4)`                | **C**onditionally **j**ump                            | `if T { RIP <- addr }`                                 |
| R4-4  | `10001110` | `jcall(addr:R4)`              | **J**ump: **Call** subroutine                         | `rps <- rps - 4; mem32(rps) <- rip; rip <- addr`       |
| -     | `10001111` | `jret()`                      | **J**ump: **Ret**urn from subroutine                  | `rip <- mem32(rps); rps <- rps + 4`                    |
| R4W4  | `10001110` | `c_lr(src:R4, dst:R4)`        | **C**onditionally **l**oad **r**egister               | `if T { dst <- src }`                                  |
|       | ...        |                               | *Reserved*                                            |                                                        |
| I12   | `1001----` | `c_ji(ipoff:Id12)`            | **C**onditionally **j**ump with IP-relative **i**mm.  | `if T { rip <- rip + 4 + s32(ipoff) }`                 |
|       |            |                               | _**Arithmetic and bitwise logic**_                    |                                                        |
| W4R4  | `10100000` | `bsext8(dst: W4, a:R4)`       | **S**ign-**ext**end from **8** to 32                  | `dst <- sign extend s8(a)`                             | 
| W4R4  | `10100001` | `bsext16(dst: W4, a:R4)`      | **S**ign-**ext**end from **16** to 32                 | `dst <- sign extend s16(a)`                            |
| W4R4  | `10100010` | `ineg(dst:W4, a:R4)`          | Arithmetic **neg**ative of value                      | `dst <- (-s32(a))`                                     |
| A4R4  | `10100000` | `iadd(dst:A4, b:R4)`          | Arithmetic **add**                                    | `dst <- dst + b`                                       |
| A4R4  | `10100001` | `isub(dst:A4, b:R4)`          | Arithmetic **sub**tract                               | `dst <- dst - b`                                       |
| R    | `1010` | `band $dst $b`          | Bitwise **and**                                | `$dst = $dst & $b`                                       |
| R    | `1011` | `bor $dst $b`           | Bitwise **or**                                 | `$dst = $dst \| $b`                                      |
| R    | `1100` | `bxor $dst $b`          | Bitwise **xor**                                | `$dst = $dst ^ $b`                                       |
| R    | `1101` | `bshl $dst $b`          | Bitwise **sh**ift **l**eft                     | `$dst = $dst << $b[:5]`                                      |
| R    | `1110` | `bshr $dst $b`          | Bitwise **sh**ift **r**ight                    | `$dst = $dst >> $b[:5]`                                      |
| R    | `1110` | `ashr $dst $b`         | **A**rithmetic **sh**ift **r**ight             | `$dst = $dst >>> $b[:5]`                                     |

### Opcode ranges

### Encoding and instruction formats

Every instruction is encoded in 16 bits. Large immediate loads are intended to be handled through literal pools.

In the following table, `x` are bits ignored for a specific instruction format.
Bits belonging to neither the opcode or the instruction format **must** be `0`.

When it comes to naming:

- `Rx` refers to a *read-only* register encoded over *x* bits
- `Ax` refers to a *read-write* register encoded over *x* bits
- `Wx` refers to a *write-only* register encoded over *x* bits
- `Ixy` refers to an *immediate* value with a multiplier of *x* encoded over *y* bits, where
    - No `x` means the value is not multiplied
    - `d` means `*2` (`<<1`)
    - `q` means `*4` (`<<2`)
    - `o` means `*8` (`<<3`)
- `-x` refers to unused bits in the instruction

| Format | Operands | Instruction bits      |
|--------|----------|-----------------------|
| R4R4/R4W4/R4A4/R4-4 | a, b     | `xxxx'xxxx'bbbb'aaaa` |
| R2I8   | a, i     | `xxxx'aaii'iiii'iiii` |
| R4I8   | a, i     | `xxxx'aaaa'iiii'iiii` |
| I12    | i        | `xxxx'iiii'iiii'iiii` |

## Memory layout

TODO

### Framebuffer (bank `0xFFFF`)

TODO

Optional.

Enables paletted text rendering.

- `0x2000`..`0x2F9F`: FbChar[80][25] - framebuffer data
- `0x2FA0`..`0x2FCF`: RgbColor[16] - palette data
- `0x2FD0`: any write causes a vsync wait

FbChar bit layout:
- 0..6: ASCII char.
- 7: Bold.
- 8..11: Foreground palette entry.
- 12..15: Background palette entry.

RgbColor bit layout:
- 0..7: Red channel.
- 8..15: Green channel.
- 16..23: Blue channel.