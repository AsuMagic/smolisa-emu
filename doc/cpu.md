# smol2

## Memory

### Word size

The width of memory addresses, most ALU operands and general registers is 32
bits.

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

"Specific-purpose" registers may usually still be used as general purpose
registers in specific contexts.

Volatile registers may be altered by functions in the ABI.  
To preserve the value of volatile registers, the caller must save them.  
If a register is used as an argument for an ABI call, then the caller is free to
mutate it as if it were volatile.

| Mnemonic   | Encoding | Use or ABI meaning                            | Volatile |
|------------|----------|-----------------------------------------------|----------|
|            |          | **General purpose registers**                 |          |
| `r0`       | `0x0`    | ABI: Arg #0 or vararg count; return register  | Yes      |
| `r1`       | `0x1`    | ABI: Arg #1                                   | Yes      |
| `r2`       | `0x2`    | ABI: Arg #2                                   | Yes      |
| `r3`       | `0x3`    | ABI: Arg #3                                   | Yes      |
| `r4`       | `0x4`    | ABI: Arg #4                                   | Yes      |
| `r5`       | `0x5`    | ABI: Arg #5                                   | If arg   |
| `r6`       | `0x6`    | ABI: Arg #6                                   | If arg   |
| `r7`       | `0x7`    |                                               | No       |
| `r8`       | `0x8`    |                                               | No       |
| `r9`       | `0x9`    |                                               | No       |
| `r10`      | `0xA`    |                                               | No       |
| `r11`      | `0xB`    |                                               | No       |
| `r12`      | `0xC`    |                                               | No       |
|            |          | **Specific-purpose registers**                |          |
| `rret`     | `0xD`    | Return address from jump-and-link             | Yes      |
| `rpl`      | `0xE`    | Literal pool start address                    | No       |
| `rps`      | `0xF`    | Stack pointer                                 | No       |

Certain special registers cannot be addressed directly, but may be manipulated
and used by certain instructions.

| Name       | Width  | Meaning             |
|------------|--------|---------------------|
| `T`        | 1 bit  | **T**est bit        |
| `rip`      | 32-bit | Instruction pointer |

#### Stack

The standard way to use the stack is to initialize it to the *highest* address. The stack grows *downwards* in the address space.

Stack pushes are available as an instruction, but pops are not for
implementation difficulty reasons.

`push(r5)` is functionally equivalent to:

```python
    addi(rps, -4),
    s32(rps, r5)
```

`pop(r5)` would have to be implemented as:

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

Excluding instruction cache miss penalties, the cost of misprediction is of
1 cycle.

Conditional branches and loads read the special `T` bit register.
The `T` bit is manipulated by certain arithmetic and test instructions.

## Instructions

| Instr[8:]  |Format   | Mnemonic                           | Description                                           | Pseudocode                                             |
|------------|---------|------------------------------------|-------------------------------------------------------|--------------------------------------------------------|
|            |         |                                    | _**Loads**_                                           |                                                        |
| `00000000` | R4W4    | `l8(addr:R4, dst:W4)`              | **L**oad u**8** from memory                           | `dst <- u32(mem8(addr))`                               |
| `00000001` | R4W4    | `l16(addr:R4, dst:W4)`             | **L**oad u**16** from memory                          | `dst <- u32(mem16(addr))`                              |
| `00000010` | R4W4    | `l32(addr:R4, dst:W4)`             | **L**oad u**32** from memory                          | `dst <- mem32(addr)`                                   |
| `00000011` | R4W4    | `c_lr(src:R4, dst:W4)`             | **C**onditionally **l**oad **r**egister               | `if T { dst <- src }`                                  |
| `00000100` | R4W4E16 | `l8ow(base:R4, dst:W4, off:E16)`   | **L**oad u**8** with **o**ffset (**w**ide) ±32K       | `dst <- u32(mem8(base + s32(imm)))`                    |
| `00000101` | R4W4E16 | `l16ow(base:R4, dst:W4, off:E16)`  | **L**oad u**16** with **o**ffset (**w**ide) ±64K      | `dst <- u32(mem16(base + s32(imm) << 1))`              |
| `00000110` | R4W4E16 | `l32ow(base:R4, dst:W4, off:E16)`  | **L**oad u**32** with **o**ffset (**w**ide) ±128K     | `dst <- mem32(base + s32(imm) << 2))`                  |
| `00000111` | R4W4    | `lr(src:R4, dst:R4)`               | **L**oad **r**egister                                 | `dst <- src`                                           |
| `00001000` | R4W4    | `ls8(addr:R4, dst:W4)`             | **L**oad **s8** from memory                           | `dst <- s32(mem8(addr))`                               |
| `00001001` | R4W4    | `ls16(addr:R4, dst:W4)`            | **L**oad **s16** from memory                          | `dst <- s32(mem16(addr))`                              |
| `00001010` | R4W4E16 | `ls8ow(base:R4, dst:W4, off:E16)`  | **L**oad **s8** with **o**ffset (**w**ide) ±32K       | `dst <- s32(mem8(base + u32(imm)))`                    |
| `00001011` | R4W4E16 | `ls16ow(base:R4, dst:W4, off:E16)` | **L**oad **s16** with **o**ffset (**w**ide) ±64K      | `dst <- s32(mem16(base + u32(imm) << 1))`              |
| `000011--` | Rh2W2I6 | `l8o(base:Rh2, dst:W2, off:I6)`    | **L**oad u**8** with **o**ffset <=63                  | `dst <- u32(mem8(base + u32(imm)))`                    |
| `000100--` | Rh2W2I6 | `l16o(base:Rh2, dst:W2, off:I6)`   | **L**oad u**16** with **o**ffset <=126                | `dst <- u32(mem16(base + u32(imm) << 1))`              |
| `000101--` | Rh2W2I6 | `l32o(base:Rh2, dst:W2, off:I6)`   | **L**oad u**32** with **o**ffset <=189                | `dst <- mem32(base + u32(imm) << 2))`                  |
| `000110--` | Rh2W2I6 | `ls8o(base:Rh2, dst:W2, off:I6)`   | **L**oad u**8** with **o**ffset <=63                  | `dst <- s32(mem8(base + u32(imm)))`                    |
| `000111--` | Rh2W2I6 | `ls16o(base:Rh2, dst:W2, off:I6)`  | **L**oad u**16** with **o**ffset <=126                | `dst <- s32(mem16(base + u32(imm) << 1))`              |
| `0010----` | W4I8    | `lsi(imm:I8, dst:W4)`              | **L**oad from **s**8 **i**mmediate                    | `dst <- s32(imm)`                                      |
| `0011----` | W4I8    | `lsih(imm:I8, dst:W4)`             | **L**oad from **s**8 **i**mmediate to **h**igh byte   | `dst[24:31] <- u8(imm)`                                |
| `0100----` | W4I8E16 | `lsiw(imm:I24, dst:W4)`            | **L**oad from **s**24 **i**mmediate (**w**ide)        | `dst <- s32(imm)`                                      |
| `0101----` | W4I8E16 | `liprel(imm:I24, dst:W4)`          | **L**oad **ip-rel**ative address ±16MB                | `r0 <- rip + 2 + s32(dst << 1)`                        |
|            |         |                                    | _**Stores**_                                          |                                                        |
| `01100000` | R4R4    | `s8(addr:R4, src:R4)`              | **S**tore u**8** to memory                            | `mem8(addr) <- src[:7]`                                |
| `01100001` | R4R4    | `s16(addr:R4, src:R4)`             | **S**tore u**16** to memory                           | `mem16(addr) <- src[:15]`                              |
| `01100010` | R4R4    | `s32(addr:R4, src:R4)`             | **S**tore u**32** to memory                           | `mem32(addr) <- src`                                   |
| `01100011` | R4-4    | `push(src:R4)`                     | **Push** to rps                                       | `rps <- rps - 4; mem32(rps) <- src`                    |
| `01100100` | R4R4E16 | `s8ow(base:R4, src:R4, imm:E16)`   | **S**tore u**8** with **o**ffset (**w**ide) ±32K      | `mem8(addr + s32(imm)) <- src[:7]`                     |
| `01100101` | R4R4E16 | `s16ow(base:R4, src:R4, imm:E16)`  | **S**tore u**16** with **o**ffset (**w**ide) ±64K     | `mem16(addr + s32(imm) << 1)) <- src[:15]`             |
| `01100110` | R4R4E16 | `s32ow(base:R4, src:R4, imm:E16)`  | **S**tore u**32** with **o**ffset (**w**ide) ±128K    | `mem32(addr + s32(imm) << 2)) <- src`                  |
| `01100111` |         | `brk`                              | Implementation-defined **br**ea**k**                  | N/A                                                    |
| `011010--` | Rh2R2I6 | `s8o(base:Rh2, src:R2, off:I6)`    | **S**tore u**8** with **o**ffset <= 63                | `mem8(base + u32(imm)) <- src[:7]`                     |
| `011011--` | Rh2R2I6 | `s16o(base:Rh2, src:R2, off:I6)`   | **S**tore u**16** with **o**ffset <= 126              | `mem16(base + u32(imm)) <- src[:15]`                   |
| `011100--` | Rh2R2I6 | `s32o(base:Rh2, src:R2, off:I6)`   | **S**tore u**32** with **o**ffset <= 189              | `mem32(base + u32(imm)) <- src`                        |
|            |         |                                    | _**Tests and `T`-bit manipulation**_                  |                                                        |
| `01110100` | R4R4    | `tltu(a:R4, b:R4)`                 | **T**est if **l**ower **t**han (**u**nsigned)         | `T <- (a < b)`                                         |
| `01110101` | R4R4    | `tlts(a:R4, b:R4)`                 | **T**est if **l**ower **t**han (**s**igned)           | `T <- (s32(a) < s32(b))`                               |
| `01110110` | R4R4    | `tgeu(a:R4, b:R4)`                 | **T**est if **g**reater or **e**qual (**u**nsigned)   | `T <- (a >= b)`                                        |
| `01110111` | R4R4    | `tges(a:R4, b:R4)`                 | **T**est if **g**reater or **e**qual (**s**igned)     | `T <- (s32(a) >= s32(b))`                              |
| `01111000` | R4R4    | `te(a:R4, b:R4)`                   | **T**est if **e**qual to                              | `T <- (a == b)`                                        |
| `01111001` | R4R4    | `tne(a:R4, b:R4)`                  | **T**est if **n**ot **e**qual to                      | `T <- (a != b)`                                        |
| `01111010` | R4I4    | `tltsi(a:R4, b:I4)`                | **T**est if **l**ower **t**han **s**igned **i**mm.    | `T <- (s32(a) < s32(b))`                               |
| `01111011` | R4I4    | `tgesi(a:R4, b:I4)`                | **T**est if **g**reater or **e**qual (**s**igned)     | `T <- (s32(a) >= s32(b))`                              |
| `01111100` | R4I4    | `tei(a:R4, b:I4)`                  | **T**est if **e**qual to **i**mmediate                | `T <- (a == b)`                                        |
| `01111101` | R4I4    | `tnei(a:R4, b:I4)`                 | **T**est if **n**ot **e**qual to **i**mmediate        | `T <- (a != b)`                                        |
| `01111110` | R4      | `tbz(a:R4)`                        | **T**est if any **b**yte is **z**ero                  | TODO                                                   |
| `01111111` |         | hole                               |                                                       |                                                        |
|            |         |                                    | _**Pool loads**_                                      |                                                        |
| `1000----` | W4I8    | `pl_l32(dst:R4, imm:U8)`           | r**pl**: **L**oad u**32** from memory with off <=1K   | `dst <- mem32(rpl + (u32(imm) << 2))`                  |
|            |         |                                    | _**Branching and conditional ops**_                   |                                                        |
| `10010000` | R4-4    | `j(addr:R4)`                       | **J**ump unconditionally                              | `rip <- addr`                                          |
| `10010001` | R4-4    | `c_j(addr:R4)`                     | **C**onditionally **j**ump                            | `if T { RIP <- addr }`                                 |
| `10010010` | R4W4    | `jal(addr:R4, target:R4)`          | **J**ump **a**nd **l**ink                             | `ret <- rip + 2; rip <- addr`                          |
| ...        |         |                                    | *Reserved*                                            |                                                        |
| `1010----` | I12E16  | `jali(ipoff:I28)`                  | **J**ump **a**nd **l**ink to **i**mmediate ±128M      | `ret <- rip + 2; rip <- rip + 2 + s32(ipoff)`          |
| `1011----` | I12     | `c_ji(ipoff:I12)`                  | **C**onditionally **j**ump with IP-relative **i**mm.  | `if T { rip <- rip + 2 + s32(ipoff) }`                 |
|            |         |                                    | _**Arithmetic and bitwise logic**_                    |                                                        |
| `11000000` | W4R4    | `bsext8(dst: W4, a:R4)`            | **S**ign-**ext**end from **8** to 32                  | `dst <- sign extend s8(a)`                             | 
| `11000001` | W4R4    | `bsext16(dst: W4, a:R4)`           | **S**ign-**ext**end from **16** to 32                 | `dst <- sign extend s16(a)`                            |
| `11000010` | W4R4    | `bzext8(dst: W4, a:R4)`            | **Z**ero-**ext**end from **8** to 32                  | `dst <- zero extend s8(a)`                             | 
| `11000011` | W4R4    | `bzext16(dst: W4, a:R4)`           | **Z**ero-**ext**end from **16** to 32                 | `dst <- zero extend s16(a)`                            |
| `11000100` | W4R4    | `ineg(dst:W4, a:R4)`               | Integer **neg**ative of value                         | `dst <- (-s32(a))`                                     |
| `11000101` | A4R4    | `isub(dst:A4, b:R4)`               | Integer **sub**tract                                  | `dst <- dst - b`                                       |
| `11000110` | A4R4    | `iadd(dst:A4, b:R4)`               | Integer **add**                                       | `dst <- dst + b`                                       |
| `11000111` | A4I4    | `iaddsi(dst:A4, b:I4)`             | Integer **add s**igned **i**mmediate                  | `dst <- dst + s32(b)`                                  |
| `11001000` | A4R4E16 | `iaddsiw(dst:A4, a:R4, b:E16)`     | Integer **add s**igned **i**mmediate (**w**ide)       | `dst <- a + s32(b)`                                    |
| `11001001` | A4I4    | `iaddsi_tnz(dst:A4, b:I4)`         | Integer **add s**igned **i**mm. then **t**est for **n**on-**z**ero | `dst <- dst + s32(b); T <- (dst != 0)`    |
| `11001010` | A4R4    | `band(dst:A4, b:R4)`               | Bitwise **AND**                                       | `dst <- dst & b`                                       |
| `11001011` | A4R4    | `bor(dst:A4, b:R4)`                | Bitwise **OR**                                        | `dst <- dst \| b`                                      |
| `11001100` | A4R4    | `bxor(dst:A4, b:R4)`               | Bitwise **XOR**                                       | `dst <- dst ^ b`                                       |
| `11001101` | A4R4    | `bsl(dst:A4, b:R4)`                | Bitwise **s**hift **l**eft                            | `dst <- dst << b[:5]`                                  |
| `11001110` | A4R4    | `bsr(dst:A4, b:R4)`                | Bitwise **s**hift **r**ight (pads `0`s)               | `dst <- dst >> b[:5]`                                  |
| `11001111` | A4R4    | `basr(dst:A4, b:R4)`               | Integer **a**rith. **s**hift **r**ight (pads sign)    | `dst <- dst >>> b[:5]`                                 |
| `1101000-` | A4I5    | `bsli(dst:A4, b:I5)`               | Bitwise **s**hift **l**eft with **i**mmediate         | `dst <- dst << b`                                      |
| `1101001-` | A4I5    | `bsri(dst:A4, b:I5)`               | Bitwise **s**hift **r**ight with **i**mmediate        | `dst <- dst >> b`                                      |
| `1101010-` | A4I5    | `basri(dst:A4, b:I5)`              | Integer **a**rith. **s**hift **r**ight with **i**mm.  | `dst <- dst >>> b`                                     |
| `11010110` |         | hole                               |                                                       |                                                        |
| `11010111` |         | hole                               |                                                       |                                                        |
| `11011---` |         | hole                               |                                                       |                                                        |
| `111-----` |         | hole                               |                                                       |                                                        |

### Opcode ranges

### Encoding and instruction formats

Most instructions are encoded in 16 bits. The instruction type can always be
decoded with the first 8 bits.

Certain instructions may encode immediates at least partly over the 16 or 32
bits following the instruction.  
Each 16 bits of extra immediates, assuming no cache miss, imply +1 cycle of
latency.

In the following table:

- `x` are bits ignored for a specific instruction format.
- `o` are bits belonging to the opcode.
- `(h)` (optional) stands for High meaning that the most significant bit is to
be set when indexing a register (when `x < 4 bits`), e.g.
    - `R2` indexes `(r0, r1, r2, r3)`
    - `Rh2` indexes `(r8, r9, r10, r11)`

Bits belonging to neither the opcode or the instruction format **must** be `0`.

When it comes to naming:

- `R(h)x` refers to a *read-only* register encoded over *x* bits
- `A(h)x` refers to a *read-write* register encoded over *x* bits
- `W(h)x` refers to a *write-only* register encoded over *x* bits
- `Ix` refers to an *immediate* value encoded over *x* bits
- `-x` refers to unused bits in the instruction
- `Ex` refers to **extra** *x* immediate bits encoded starting at `rip+2`.
These bits are always skipped when fetching the next instruction.
Immediates present in the main instruction `u16` represent the higher bits if
present, whereas `Ex` will represent the lower bits.

In the following table, `A` and `W` are always replaced with `R` as the
encodings would be otherwise equivalent.

| Format  | Operands | Next two bytes        | Instruction bits      |
|---------|----------|-----------------------|-----------------------|
| R4      | a        | x                     | `oooo'oooo'xxxx'aaaa` |
| R4R4    | a, b     | x                     | `oooo'oooo'bbbb'aaaa` |
| R4I4    | a, i     | x                     | `oooo'oooo'iiii'aaaa` |
| R4I8    | a, i     | x                     | `oooo'iiii'iiii'aaaa` |
| R4I8E16 | a, i     | `iiii'iiii'iiii'iiii` | `oooo'iiii'iiii'aaaa` |
| R4R4E16 | a, b, i  | `iiii'iiii'iiii'iiii` | `oooo'oooo'bbbb'aaaa` |
| Rh2R2I6 | a, b, i  | x                     | `oooo'ooii'iiii'bbaa` |
| I12     | i        | x                     | `oooo'iiii'iiii'iiii` |
| I12E16  | i        | `iiii'iiii'iiii'iiii` | `oooo'iiii'iiii'iiii` |

## Memory layout

- `0x00000000`..`0xEFFFFFFF`: RAM
- `0xF0002000`..`0xF0002FFF`: Framebuffer

### Framebuffer (`0xF0002000`)

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