# smolisa

## Words

### Word size

CPU words are 16-bit. The registers, ALU input and output and the memory address size are as such 16-bit.

### Endianness

The architecture is **little-endian**.

## Registers

All registers initialize to zero by default, including the instruction pointer.

- `$g0` .. `$g14`: 16-bit general purpose registers
- `$bank`: Bank select (see Bank Swapping)
- Instruction pointer: Internal pointer

## Instruction cheatsheet

| Type | Bits   | Mnemonic              | Meaning                                        | Operation                                    |
|------|--------|-----------------------|------------------------------------------------|----------------------------------------------|
|      |        |                       | **Data transfer**                              |                                              |
| I    | `0000` | `li $dst imm8`        | **L**oad **i**mmediate to 8 lower bits         | `$dst[0:7] = imm8`                           |
| R    | `0001` | `lm $addr $dst`       | **L**oad **m**emory to 8 lower bits            | `$dst[0:7] = mem[$addr]`                     |
| R    | `0010` | `sm $addr $src`       | **S**tore 8 lower bits to **m**emory           | `mem[$addr] = $src[0:7]`                     |
|      |        |                       | **Branching**                                  |                                              |
| R    | `0011` | `b $addr`             | **B**ranch                                     | `pc = $addr`                                 |
| R    | `0100` | `bz $addr $a`         | **B**ranch if **z**ero                         | `if $a == 0: pc = $addr`                     |
| R    | `0101` | `bnz $addr $a`        | **B**ranch if **n**ot **z**ero                 | `if $a != 0: pc = $addr`                     |
|      |        |                       | **Arithmetic**                                 |                                              |
| R    | `0110` | `add $dst $a $b`      | Arithmetic **add**                             | `$dst = $a + $b`                             |
| R    | `0111` | `sub $dst $a $b`      | Arithmetic **sub**tract                        | `$dst = $a - $b`                             |
| R    | `1000` | `not $dst $a`         | Bitwise **not**                                | `$dst = ~$a`                                 |
| R    | `1001` | `and $dst $a $b`      | Bitwise **and**                                | `$dst = $a & $b`                             |
| R    | `1010` | `or $dst $a $b`       | Bitwise **or**                                 | `$dst = $a | $b`                             |
| R    | `1011` | `xor $dst $a $b`      | Bitwise **xor**                                | `$dst = $a ^ $b`                             |
| R    | `1100` | `shl $dst $a $b`      | Bitwise **sh**ift **l**eft                     | `$dst = $a << $b`                            |
| R    | `1101` | `shr $dst $a $b`      | Bitwise **sh**ift **r**ight                    | `$dst = $a >> $b`                            |
| R    | `1110` | `swb $dst $a`         | **Sw**ap **b**ytes                             | `$dst[0:7] = $a[8:15]; $dst[8:15] = $a[0:7]` |
|      |        |                       | **Reserved**                                   |                                              |
|      | `1111` |                       |                                                |                                              |

## Opcodes

Opcodes are a fixed 16-bit field.

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

### Encoding examples

```
; Example: loading a full 16-bit value '0xABCD' to a register

; TYPE-I      -r1- -op- ---imm8--
; TYPE-R      -r1- -op- -r3- -r2-

; $g6 = 0xABCD
li $g6 0xAB ; 0110'0000 1010'1011
swb $g6 $g6 ; 0110'1110 xxxx'0110
li $g6 0xCD ; 0110'0000 1100'1101
```

```
; Example: set bank to 0xF0F0, try to read from it and blow up
; This is really crappy, as it will try to set $bank in two passes which isnt exactly good

; TYPE-I          -r1- -op- ---imm8--
; TYPE-R          -r1- -op- -r3- -r2-

; set bank 0xF0F0
li $bank 0xF0 ;   1111'0000 1111'0000
swb $bank $bank ; 1111'1110 xxxx'1111
li $bank 0xF0 ;   1111'0000 1111'0000

; load address 0xF000 to $g0
li $g0 0xF0 ;     0000'0000 1111'0000
swb $g0 $g0 ;     0000'1110 xxxx'0000
li $g0 0x00 ;     0000'0000 0000'0000

; jump to 0xF000
b $g0 ;           0000'0011 xxxx'xxxx
```

```
; Example: jump to unaligned memory

; TYPE-I          -r1- -op- ---imm8--
; TYPE-R          -r1- -op- -r3- -r2-

; clear $g0, load 0x01
xor $g0 $g0 $g0 ; 0000'1011 0000'0000
li $g0 0x01 ;     0000'0000 0000'0001

; jump to 0x0001
b $g0 ;           0000'0011 xxxx'xxxx
```

```
; Example: write 'a' to the top left of the framebuffer
;          note: this assumes an emulator and/or firmware that initializes:
;                - the palette;
;                - character color codes;
;                otherwise nothing will be shown on screen.

; TYPE-I                -r1- -op- ---imm8--
; TYPE-R                -r1- -op- -r3- -r2-

; set bank 0xFFFF
xor $bank $bank $bank ; 1111'1011 1111'1111
not $bank $bank       ; 1111'1000 xxxx'1111

; load address 0x2000 to $g0
li $g0 0x20           ; 0000'0000 0010'0000
swb $g0 $g0           ; 0000'1110 xxxx'xxxx
li $g0 0x00           ; 0000'0000 0000'0000

; load character 'a' to $g1
; we dont care about the upper byte
li $g1 'a'            ; 0001'0000 0110'0001

; copy 'a' to addr $g0
sm $g0 $g1            ; 0000'0010 xxxx'0001
```

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