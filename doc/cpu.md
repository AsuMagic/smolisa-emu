# smolisa

## Words

### Word size

CPU words are 16-bit. ALU ops operate over 16-bit registers.  
Memory reads and writes can be done up to 16 bits at a time.

### Memory access

The architecture performs memory accesses in a little-endian fashion. Opcodes should be laid out with this in mind as well.

Bank swapping of the last 48KiB is implemented through the `$bank` register, which extends the theoretical maximum available memory to over 3GiB.  
However, there is no guarantee as to how many banks are provided on a specific implementation, if any.

16-bit memory accesses must be aligned to 16-bit, or undefined behavior will occur. As such, any address used in `lw` or `sw` must be aligned. Opcodes must also be aligned.

## Registers

All registers are initialized to zero on reset, including the instruction pointer. 
The firmware should as a result be loaded from address `0x0000`.

- `$g0` .. `$g13`: 16-bit general purpose registers
- `$ip`: Instruction pointer. Contains the instruction pointer to the next instruction, not to the currently executed one. Can be written to for branching.
- `$bank`: Bank select (see Bank Swapping)

## Instruction cheatsheet

| Type | Bits   | Mnemonic               | Meaning                                        | Operation                                              |
|------|--------|------------------------|------------------------------------------------|--------------------------------------------------------|
|      |        |                        | **Data transfer**                              |                                                        |
| I    | `0000` | `li $dst imm8`         | **L**oad **i**mmediate to 8 lower bits         | `$dst[0:7] = imm8`                                     |
| I    | `0001` | `liu $dst imm8`        | **L**oad **i**mmediate to 8 **u**pper bits     | `$dst[8:15] = imm8`                                    |
| R    | `0010` | `lb $addr $dst`        | **L**oad **b**yte from memory                  | `$dst[0:7] = mem[$addr]`                               |
| R    | `0011` | `sb $addr $src`        | **S**tore **b**yte to memory                   | `mem[$addr] = $src[0:7]`                               |
| R    | `0100` | `lw $addr $dst`        | **L**oad **w**ord from memory (little-endian)  | `$dst[0:7] = mem[$addr]; $dst[8:15] = mem[$addr + 1];` |
| R    | `0101` | `sw $addr $src`        | **S**tore **w**ord to memory (little-endian)   | `mem[$addr] = $src[0:7]; mem[$addr + 1] = $src[8:15];` |
|      |        |                        | **Conditional load**                           |                                                        |
| R    | `0110` | `lrz $dst $src $cond`  | **L**oad **r**egister if **z**ero              | `if $cond == 0 { $dst = $src }`                        |
| R    | `0111` | `lrnz $dst $src $cond` | **L**oad **r**egister if **n**ot **z**ero      | `if $cond != 0 { $dst = $src }`                        |
|      |        |                        | **Arithmetic**                                 |                                                        |
| R    | `1000` | `add $dst $a $b`       | Arithmetic **add**                             | `$dst = $a + $b`                                       |
| R    | `1001` | `sub $dst $a $b`       | Arithmetic **sub**tract                        | `$dst = $a - $b`                                       |
| R    | `1010` | `and $dst $a $b`       | Bitwise **and**                                | `$dst = $a & $b`                                       |
| R    | `1011` | `or $dst $a $b`        | Bitwise **or**                                 | `$dst = $a | $b`                                       |
| R    | `1100` | `xor $dst $a $b`       | Bitwise **xor**                                | `$dst = $a ^ $b`                                       |
| R    | `1101` | `shl $dst $a $b`       | Bitwise **sh**ift **l**eft                     | `$dst = $a << $b`                                      |
| R    | `1110` | `shr $dst $a $b`       | Bitwise **sh**ift **r**ight                    | `$dst = $a >> $b`                                      |
| R    | `1111` | `swb $dst $a $b`       | **Sw**ap **b**ytes                             | `$dst[0:7] = $a[8:15]; $dst[8:15] = $b[0:7]`           |

## Opcodes

Opcodes have a 16-bit fixed length.

### Common

- 0..3: Instruction
- 4..7: Register `$1`

### Type-R(egister)

Provides 3 registers to address in total.

- 8..11: Register `$2`
- 12..15: Register `$3`

### Type-I(mmediate):

Provides 1 register to address plus a 8-bit immediate.

- 8..15: Immediate `imm8`

## Memory layout

- `0x0000`..`0x1FFF`: Fixed memory
- `0x2000`..`0xFFFF`: Bank-swappable memory

### Bank swapping

Register `$bank` selects which 48KiB segment to access for any memory access within that region.  
Bank `0xFFFF` is always available and selected for MMIO.  
Bank `0x0000` is reserved and indicates an invalid page. Any access will cause a CPU halt.  
Not all banks may be available (in fact, there can be none), in which case setting the `$bank` register will result into it containing bank `0x0000` instead.

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