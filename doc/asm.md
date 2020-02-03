# smolisa-as

## Usage

```sh
smolisa-as /path/to/assembler >./output.img
```

## Syntax

### Comments

Comments begin with `;` and end at the end of the same line.  
Comments are completely ignored by the assembler.

### Instructions

There should be one instruction or assembler directive per line.  
The syntax is the same as described in the "mnemonic" columns of [the instruction set documentation](cpu.md).

Example:

```
; r1 = r2 + r3
add $r1 $r2 $r3
```

### Immediate values

Numeric literals can be used when immediate values are expected as operands of an instruction or of an assembler directive.

These can be written in several ways:
- Decimal, e.g. `1234`
- Hexadecimal, e.g. `0xDEAD` (prefixed with `0x`)
- ASCII, e.g. `'a'` for ASCII 97

### Labels

Label declarations allows to refer to the address of an instruction or to other data by a name. They are declared using an identifier followed by a colon `:`.

Because immediates are only 1 byte long at most within an instruction, label uses must refer to a specific byte of the label.

e.g. an infinite loop:

```
label:
liu $r1 label~high ; Load the 8 upper bits of label into $r1
li $r1 label~low   ; Load the 8 lower bits of label into $r2
or $rip $r1 $r1    ; Jump into label
```

NOTE: at the moment, it is impossible to obtain the bank address of data within the image.


### Assembler directives

Assembler directives affect the state of the assembler. They are prefixed with `#` and must appear within a single line.

#### `#offset (immediate offset)`

Specifies the offset of the next data to be written.

e.g. to write at offset `0x2000` of the output image:

```
#offset 0x2000
```

#### `#binary (string literal)`

Dump a binary file into the output image at the current offset.

```
#binary "./mydata.bin"
```