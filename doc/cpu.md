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

All general-purpose registers are initialized to zero on reset.

The instruction pointer initializes in an implementation-defined manner (typically `0x0`).
TODO: this should not really be the case, memory mapping will make this make more sense

| Mnemonic   | Encoding | Mode | Meaning                                       |
|------------|----------|------|-----------------------------------------------|
|            |          |      | **Regular registers**                         |
| `R0`       | `0x0`    | RW   | General purpose register                      |
| ...        |          |      |                                               |
| `R11`      | `0xB`    | RW   |                                               |
|            |          |      | **Pool address registers**                    |
| `RPL`      | `0xC`    | RW   | Literal pool start address                    |
| `RP0`      | `0xD`    | RW   | User pool start address                       |
| Reserved   | `0xE`    | noop | TODO |
| Reserved   | `0xF`    | noop | TODO |

#### Stack

The standard way to use the stack is to initialize it to the *highest* address. The stack grows *downwards* in the address space.

For instance, `PUSH(RG5)` is roughly equivalent to:

```python
    add_s(RPS, -4),
    sq(RPS, R5)
```

... and `POP(R5)` is roughly equivalent to:

```python
    lq(RPS, R5),
    add_s(RPS, 4)
```

#### The stack and interrupts

The stack should be set up in a consistent state before interrupts (TODO) are enabled.

When an interrupt fires, the CPU will dump registers to the stack. Memory below the stack pointer should be expected to be mutated at any time.

## Instructions

TODO:

- sext8
- sext16
- branch thru a flag or have separate ops?
- have conditional loads or not?
- swap8
- swap16
- check more of what SH might have interesting
- check more of what RV might have interesting
- check more of what Thumb might have interesting
- reserve some insns still

|Format | Instr[0:7] | Mnemonic                      | Description                                           | Pseudocode                                             |
|-------|------------|-------------------------------|-------------------------------------------------------|--------------------------------------------------------|
|       |            |                               | _**Loads**_                                           |                                                        |
| R4R4  | `00000000` | `lu8(addr:R4, dst:R4)`        | **L**oad **u8** from memory (zext)                    | `dst <- u32(mem8(addr))`                               |
| R4R4  | `00000001` | `lu16(addr:R4, dst:R4)`       | **L**oad **u16** from memory (zext)                   | `dst <- u32(mem16(addr))`                              |
| R4R4  | `00000010` | `lu32(addr:R4, dst:R4)`       | **L**oad **u32** from memory                          | `dst <- mem32(addr)`                                   |
| R4    | `00000011` | `pop(src:R4)`                 | **Pop** from stack (PS)                               | `dst <- mem32(PS); PS <- PS + 4`                       |
| I6R4  | `000001--` | `lou8_r0(addr:R4, imm:I6)`    | **L**oad **o**ffset **u8** from memory to **R0**      | `dst <- u32(mem8(addr + u32(imm)))`                    |
| I6R4  | `000010--` | `lou16_r0(addr:R4, imm:I6)`   | **L**oad **o**ffset **u16** from memory to **R0**     | `dst <- u32(mem16(addr + u32(imm) << 1))`              |
| I6R4  | `000011--` | `lou32_r0(addr:R4, imm:I6)`   | **L**oad **o**ffset **u32** from memory to **R0**     | `dst <- mem32(addr + u32(imm) << 2))`                  |
| R4I8  | `0001----` | `lsi(imm:I8, dst:R4)`         | **L**oad from **s**8 **i**mmediate                    | `dst <- s32(imm)`                                      |
| I12   | `0010----` | `lsi_r0(imm:I12)`             | **L**oad from **s**12 **i**mmediate to **R0**         | `R0 <- s32(imm)`                                       |
|       |            |                               | _**Stores**_                                          |                                                        |
| R4R4  | `00110000` | `su8(addr:R4, src:R4)`        | **S**tore **u8** to memory                            | `mem8(addr) <- src[:7]`                                |
| R4R4  | `00110001` | `su16(addr:R4, src:R4)`       | **S**tore **u16** to memory                           | `mem16(addr) <- src[:15]`                              |
| R4R4  | `00110010` | `su32(addr:R4, src:R4)`       | **S**tore **u32** to memory                           | `mem32(addr) <- src`                                   |
| R4    | `00110011` | `push(src:R4)`                | **Push** to PS                                        | `PS <- PS - 4; mem32(PS) <- src`                       |
| I6R4  | `001101--` | `sou8_r0(addr:R4, src:R4)`    | **S**tore **o**ffset **u8** to memory from **R0**     | `mem8(addr + u32(imm)) <- src[:7]`                     |
| I6R4  | `001110--` | `sou16_r0(addr:R4, src:R4)`   | **S**tore **o**ffset **u16** to memory from **R0**    | `mem16(addr + u32(imm) << 1)) <- src[:15]`             |
| I6R4  | `001111--` | `sou32_r0(addr:R4, src:R4)`   | **S**tore **o**ffset **u32** to memory from **R0**    | `mem32(addr + u32(imm) << 2)) <- src`                  |
|       |            |                               | _**Pool loads**_                                      |                                                        |
| R4I8  | `0100----` | `pl_lu32(dst:R4, imm:Iq8)`    | **PL** **L**oad **u32** from memory ±1K               | `dst <- mem32(RP0 + (u32(imm) << 2))`                  |
| R4I8  | `0101----` | `p0_lu32(dst:R4, imm:Iq8)`    | **P0** **L**oad **u32** from memory ±1K               | `dst <- mem32(RPG + (u32(imm) << 2))`                  |
| I12   | `0110----` | `p0_lu32_r0(imm:Iq12)`        | **P0** **L**oad **u32** from memory ±16K to **R0**    | `R0 <- mem32(RPG + (u32(imm) << 2))`                   |
|       |            |                               | _**Pool stores**_                                     |                                                        |
| R4I8  | `0111----` | `p0_su32(dst:R4, imm:Iq8)`    | **P0** **S**tore **u32** from memory ±1K              | `mem32(RP0 + (imm << 2)) <- src`                       |
| I12   | `1000----` | `p0_su32_r0(imm:Iq12)`        | **P0** **S**tore **u32** from memory ±16K from **R0** | `mem32(RP0 + (imm << 2)) <- R0`                        |
|       |            |                               | _**Pool IP-relative init**_                        |                                                        |
| I12   | `1001----` | `pl_pcrel(imm:Iq12)`          | **PL** **Init**ialize to IP-relative immediate     | `RPL <- IP + (imm << 2)`                               |
|       |            |                               | _**Branching**_                                    |                                                        |
| R4    | `10100000` | `j(addr:R4)`                  | **J**ump unconditionally                           | `RIP <- addr`                                          |
| R4R4  |TODO `0010001` | `jz(addr:R4, cond:R4)`   | **J**ump if **z**ero                           | `if cond == 0 { RIP <- addr }`                         |
| R4R4  | `0010010` | `jnz(addr:R4, cond:R4)`  | **J**ump if **n**ot **z**ero                   | `if cond != 0 { RIP <- addr }`                         |
| R4    | `0010100` | `jcall(addr:R4)`         | **J**ump: **Call** subroutine                  | `PS <- PS - 4; mem32(PS) <- RIP; RIP <- addr`          |
| -     | `0010101` | `jret()`                 | **J**ump: **Ret**urn from subroutine           | `RIP <- mem32(PS); PS <- PS + 4`                       |
|       |           |                          | _**Binary operators**_                         |                                                        |
| R4R5  | `1100000` | `add $dst $b`            | Arithmetic **add**                             | `dst <- dst + b`                                       |
| R4R5  | `1100001` | `sub $dst $b`            | Arithmetic **sub**tract                        | `dst <- dst - b`                                       |
| R    | `1010` | `band $dst $b`          | Bitwise **and**                                | `$dst = $dst & $b`                                       |
| R    | `1011` | `bor $dst $b`           | Bitwise **or**                                 | `$dst = $dst \| $b`                                      |
| R    | `1100` | `bxor $dst $b`          | Bitwise **xor**                                | `$dst = $dst ^ $b`                                       |
| R    | `1101` | `bshl $dst $b`          | Bitwise **sh**ift **l**eft                     | `$dst = $dst << $b[:5]`                                      |
| R    | `1110` | `bshr $dst $b`          | Bitwise **sh**ift **r**ight                    | `$dst = $dst >> $b[:5]`                                      |
| R    | `1110` | `ashr $dst $b`         | **A**rithmetic **sh**ift **r**ight             | `$dst = $dst >> $b[:5]`                                      |

Certain formats may have been chosen in a context where a smaller one would make
sense (e.g. R4 instead of R5), but would be a worse fit for decoding.

### Opcode ranges

### Encoding and instruction formats

Every instruction is encoded in 16 bits. Large immediate loads are intended to be handled through literal pools.

In the following table, `x` are bits ignored for a specific instruction format.
Bits belonging to neither the opcode or the instruction format **must** be `0`.

| Format | Operands | Instruction bits      |
|--------|----------|-----------------------|
| R4     | a        | `xxxx'xxxx'xxxx'aaaa` |
| R5     | a        | `xxxx'xxxx'xxxa'aaaa` |
| R4R4   | a, b     | `xxxx'xxxx'bbbb'aaaa` |
| R4R5   | a, b     | `xxxx'xxxb'bbbb'aaaa` |
| R4I8   | a, i     | `xxxx'iiii'iiii'aaaa` |
| R5I7   | a, i     | `xxxx'iiii'iiia'aaaa` |
| I12    | i        | `xxxx'iiii'iiii'iiii` |

#### Type-R(egister)

Provides 2 registers to address in total.

- 0..4: Opcode
- 5..: Register `$2`
- 12..15: Register `$3`

#### Type-I(mmediate):

Provides 1 register to address plus a 8-bit immediate.

- 8..15: Immediate `imm8`

## Memory layout

- `0x000000000`..`0xEFFFFFFF`: 

### Framebuffer (bank `0xFFFF`)

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