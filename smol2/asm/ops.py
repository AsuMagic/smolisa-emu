from .entities import *
from .opclasses import *

def l8(addr: Reg, dst: Reg):
    """Load u8 from memory at `addr` to register `dst`"""
    return InsR4R4(0b0000_0000, a=addr, b=dst)

def l16(addr: Reg, dst: Reg):
    """Load u16 from memory at `addr` to register `dst`"""
    return InsR4R4(0b0000_0001, a=addr, b=dst)

def l32(addr: Reg, dst: Reg):
    """Load u32 from memory at `addr` to register `dst`"""
    return InsR4R4(0b0000_0010, a=addr, b=dst)

def c_lr(src: Reg, dst: Reg):
    """If the `T`-bit is set, copy value of register `src` to register `dst`"""
    return InsR4R4(0b0000_0011, a=src, b=dst)

def l8ow(base: Reg, dst: Reg, off: int | Absolute):
    """Load u8 from memory at `addr + off` to register `dst`"""
    return InsR4R4E16(0b0000_0100, a=base, b=dst, imm=Immediate(off, signed=True, shift=0))

def l16ow(base: Reg, dst: Reg, off: int | Absolute):
    """Load u16 from memory at `addr + off` to register `dst`"""
    return InsR4R4E16(0b0000_0101, a=base, b=dst, imm=Immediate(off, signed=True, shift=1))

def l32ow(base: Reg, dst: Reg, off: int | Absolute):
    """Load u32 from memory at `addr + off` to register `dst`"""
    return InsR4R4E16(0b0000_0110, a=base, b=dst, imm=Immediate(off, signed=True, shift=2))

def lr(src: Reg, dst: Reg):
    """Copy value of register `src` to register `dst`"""
    return InsR4R4(0b0000_0111, a=src, b=dst)

def ls8(addr: Reg, dst: Reg):
    """Load u8 from memory at `addr` to `dst`, sign-extended to s32"""
    return InsR4R4(0b0000_1000, a=addr, b=dst)

def ls16(addr: Reg, dst: Reg):
    """Load u16 from memory at `addr` to `dst`, sign-extended to s32"""
    return InsR4R4(0b0000_1001, a=addr, b=dst)

def ls8ow(base: Reg, dst: Reg, off: int | Absolute):
    """Load u8 from memory at `addr + off` to `dst`, sign-extended to s32"""
    return InsR4R4E16(0b0000_0100, a=base, b=dst, imm=Immediate(off, signed=True, shift=0))

def ls16ow(base: Reg, dst: Reg, off: int | Absolute):
    """Load u16 from memory at `addr + off` to `dst`, sign-extended to s32"""
    return InsR4R4E16(0b0000_0101, a=base, b=dst, imm=Immediate(off, signed=True, shift=1))

def l8o(base: Reg, dst: Reg, off: int | Absolute):
    """Load u8 from memory at `addr + off` to register `dst`"""
    return InsRh2R2I6(0b0000_1100, a=base, b=dst, imm=Immediate(off, signed=False, shift=0))

def l16o(base: Reg, dst: Reg, off: int | Absolute):
    """Load u16 from memory at `addr + off` to register `dst`"""
    return InsRh2R2I6(0b0001_0000, a=base, b=dst, imm=Immediate(off, signed=False, shift=1))

def l32o(base: Reg, dst: Reg, off: int | Absolute):
    """Load u32 from memory at `addr + off` to register `dst`"""
    return InsRh2R2I6(0b0001_0100, a=base, b=dst, imm=Immediate(off, signed=False, shift=2))

def ls8o(base: Reg, dst: Reg, off: int | Absolute):
    """Load u8 from memory at `addr + off` to `dst`, sign-extended to s32"""
    return InsRh2R2I6(0b0001_1000, a=base, b=dst, imm=Immediate(off, signed=False, shift=0))

def ls16o(base: Reg, dst: Reg, off: int | Absolute):
    """Load u16 from memory at `addr + off` to `dst`, sign-extended to s32"""
    return InsRh2R2I6(0b0001_1100, a=base, b=dst, imm=Immediate(off, signed=False, shift=1))

def lsi(dst: Reg, imm: int | Absolute):
    """Load u8 from immediate `imm` to register `dst`, sign-extended to s32"""
    return InsR4I8(0b0010_0000, r=dst, imm=Immediate(imm, signed=True))

def lsih(dst: Reg, imm: int | Absolute):
    """Load u8 from immediate `imm` to the highest byte of `dst`, leaving lower
    bits untouched"""
    return InsR4I8(0b0011_0000, r=dst, imm=Immediate(imm, signed=True))

def lsiw(dst: Reg, imm: int | Absolute):
    """Load u24 from immediate `imm` to register `dst`, sign-extended to s32"""
    return InsR4I8E16(0b0100_0000, r=dst, imm=Immediate(imm, signed=True))

def liprel(dst: Reg, imm: int | Relative):
    """Load `rip + 2 + imm` to register `dst`"""
    return InsR4I8E16(0b0101_0000, r=dst, imm=Immediate(imm, signed=True, shift=1))

def s8(addr: Reg, src: Reg):
    """Store u8 from register `src` to memory at `addr`"""
    return InsR4R4(0b0110_0000, a=addr, b=src)

def s16(addr: Reg, src: Reg):
    """Store u16 from register `src` to memory at `addr`"""
    return InsR4R4(0b0110_0001, a=addr, b=src)

def s32(addr: Reg, src: Reg):
    """Store u32 from register `src` to memory at `addr`"""
    return InsR4R4(0b0110_0010, a=addr, b=src)

def push(src: Reg):
    """Stack push; Decrements `rps` by 4, and stores `src` to the memory at the
    decremented address, as the stack grows downwards"""
    return InsR4(0b0110_0011, r=src)

def s8ow(base: Reg, src: Reg, imm: int | Absolute):
    """Store u8 from register `src` to memory at `addr + imm`"""
    return InsR4R4E16(0b0110_0100, a=base, b=src, imm=Immediate(imm, signed=True, shift=0))

def s16ow(base: Reg, src: Reg, imm: int | Absolute):
    """Store u16 from register `src` to memory at `addr + imm`"""
    return InsR4R4E16(0b0110_0101, a=base, b=src, imm=Immediate(imm, signed=True, shift=1))

def s32ow(base: Reg, src: Reg, imm: int | Absolute):
    """Store u32 from register `src` to memory at `addr + imm`"""
    return InsR4R4E16(0b0110_0110, a=base, b=src, imm=Immediate(imm, signed=True, shift=2))

def brk():
    """Implementation-defined debugger break instruction"""
    return RawInsU16(0b0110_0111_0000_0000)

def s8o(base: Reg, src: Reg, imm: int | Absolute):
    """Store u8 from register `src` to memory at `addr + imm`"""
    return InsRh2R2I6(0b0110_1000, a=base, b=src, imm=Immediate(imm, signed=False, shift=0))

def s16o(base: Reg, src: Reg, imm: int | Absolute):
    """Store u16 from register `src` to memory at `addr + imm`"""
    return InsRh2R2I6(0b0110_1100, a=base, b=src, imm=Immediate(imm, signed=False, shift=1))

def s32o(base: Reg, src: Reg, imm: int | Absolute):
    """Store u32 from register `src` to memory at `addr + imm`"""
    return InsRh2R2I6(0b0111_0000, a=base, b=src, imm=Immediate(imm, signed=False, shift=2))

def tltu(a: Reg, b: Reg):
    """Sets the `T`-bit to `1` if `a < b` (unsigned comparison), or to `0`
    otherwise"""
    return InsR4R4(0b0111_0100, a, b)

def tlts(a: Reg, b: Reg):
    """Sets the `T`-bit to `1` if `a < b` (signed comparison), or to `0`
    otherwise"""
    return InsR4R4(0b0111_0101, a, b)

def tgeu(a: Reg, b: Reg):
    """Sets the `T`-bit to `1` if `a >= b` (unsigned comparison), or to `0`
    otherwise"""
    return InsR4R4(0b0111_0110, a, b)

def tges(a: Reg, b: Reg):
    """Sets the `T`-bit to `1` if `a >= b` (signed comparison), or to `0`
    otherwise"""
    return InsR4R4(0b0111_0111, a, b)

def te(a: Reg, b: Reg):
    """Sets the `T`-bit to `1` if `a == b` or to `0` otherwise"""
    return InsR4R4(0b0111_1000, a, b)

def tne(a: Reg, b: Reg):
    """Sets the `T`-bit to `1` if `a != b` or to `0` otherwise"""
    return InsR4R4(0b0111_1001, a, b)

def tltsi(a: Reg, b_imm: int | Absolute):
    """Sets the `T`-bit to `1` if `a < b` (signed comparison), or to `0`
    otherwise"""
    return InsR4I4(0b0111_1010, a, imm=Immediate(b_imm, signed=True))

def tgesi(a: Reg, b_imm: int | Absolute):
    """Sets the `T`-bit to `1` if `a >= b` (signed comparison), or to `0`
    otherwise"""
    return InsR4I4(0b0111_1011, a, imm=Immediate(b_imm, signed=True))

def tei(a: Reg, b_imm: int | Absolute):
    """Sets the `T`-bit to `1` if `a == b` or to `0` otherwise"""
    return InsR4I4(0b0111_1100, a, imm=Immediate(b_imm, signed=True))

def tnei(a: Reg, b_imm: int | Absolute):
    """Sets the `T`-bit to `1` if `a != b` or to `0` otherwise"""
    return InsR4I4(0b0111_1101, a, imm=Immediate(b_imm, signed=True))

def tbz(a: Reg):
    """Sets the `T`-bit to `1` if any of the 4 bytes in `a` is `== 0` or to `0`
    otherwise"""
    return InsR4(0b0111_1110, a)

def pl_l32(dst: Reg, off: int | Absolute):
    """Literal pool load; Loads `u32` from memory at `rpl + off` to `dst`"""
    return InsR4I8(0b1000_0000, dst, imm=Immediate(off, signed=False, shift=2))

def j(addr: Reg):
    """Unconditional jump; Sets `rip` to `addr`"""
    return InsR4(0b1001_0000, addr)

def c_j(addr: Reg):
    """Conditional jump; If the `T`-bit is set, sets `rip` to `addr`"""
    return InsR4(0b1001_0001, addr)

def jal(addr: Reg, target: Reg):
    """Jump and link, typically used in procedure calls; Stores `rip + 2` to
    `target` then sets `rip` to `addr`"""
    return InsR4R4(0b1001_0010, a=addr, b=target)

def jali(imm: int | Relative):
    """Jump and link, typically used in procedure calls; Stores `rip + 2` to
    `rret` then sets `rip` to `rip + 2 + imm`"""
    return InsI12E16(0b1010_0000, imm=Immediate(imm, signed=True, shift=1))

def c_ji(imm: int | Relative):
    """Conditional jump; If the `T`-bit is set, sets `rip` to `rip + 2 + imm`"""
    return InsI12(0b1011_0000, imm=Immediate(imm, signed=True, shift=1))

def bsext8(dst: Reg, a: Reg):
    """Sign-extends the value in `a` from u8 to s32, saves result to `dst`"""
    return InsR4R4(0b1100_0000, a=dst, b=a)

def bsext16(dst: Reg, a: Reg):
    """Sign-extends the value in `a` from u16 to s32, saves result to `dst`"""
    return InsR4R4(0b1100_0001, a=dst, b=a)

def bzext8(dst: Reg, a: Reg):
    """Zero-extends the value in `a` from u8 to u32, saves result to `dst`,
    effectively clearing the 3 upper bytes"""
    return InsR4R4(0b1100_0010, a=dst, b=a)

def bzext16(dst: Reg, a: Reg):
    """Zero-extends the value in `a` from u16 to u32, saves result to `dst`,
    effectively clearing the 2 upper bytes"""
    return InsR4R4(0b1100_0011, a=dst, b=a)

def ineg(dst: Reg, a: Reg):
    """Stores `-a` to `dst`"""
    return InsR4R4(0b1100_0100, a=dst, b=a)

def isub(a_dst: Reg, b: Reg):
    """Stores `a_dst - b` to `a_dst`"""
    return InsR4R4(0b1100_0101, a=a_dst, b=b)

def iadd(a_dst: Reg, b: Reg):
    """Stores `a_dst + b` to `a_dst`"""
    return InsR4R4(0b1100_0110, a=a_dst, b=b)

def iaddsi(a_dst: Reg, b: int | Absolute):
    """Stores `a_dst + b` to `a_dst`"""
    return InsR4I4(0b1100_0111, r=a_dst, imm=Immediate(b, True, 0))

def iaddsiw(dst: Reg, a: Reg, b: int | Absolute):
    """Stores `a + b` to `dst`"""
    return InsR4R4E16(0b1100_1000, a=dst, b=a, imm=Immediate(b, True, 0))

def iaddsi_tnz(a_dst: Reg, b: int | Absolute):
    """Stores `a_dst + b` to `a_dst`. The `T`-bit is set to `1` if the result
    is `!= 0`, and to `0` otherwise"""
    return InsR4I4(0b1100_1001, r=a_dst, imm=Immediate(b, True, 0))

def band(a_dst: Reg, b: Reg):
    """Stores `a_dst & b` to `a_dst`"""
    return InsR4R4(0b1100_1010, a=a_dst, b=b)

def bor(a_dst: Reg, b: Reg):
    """Stores `a_dst | b` to `a_dst`"""
    return InsR4R4(0b1100_1011, a=a_dst, b=b)

def bxor(a_dst: Reg, b: Reg):
    """Stores `a_dst ^ b` to `a_dst`"""
    return InsR4R4(0b1100_1100, a=a_dst, b=b)

def bsl(a_dst: Reg, b: Reg):
    """Stores `a_dst << b[0:5]` to `a_dst`"""
    return InsR4R4(0b1100_1101, a=a_dst, b=b)

def bsr(a_dst: Reg, b: Reg):
    """Stores `a_dst >> b[0:5]` to `a_dst`"""
    return InsR4R4(0b1100_1110, a=a_dst, b=b)

def basr(a_dst: Reg, b: Reg):
    """Stores `a_dst >>> b[0:5]` to `a_dst`"""
    return InsR4R4(0b1100_1111, a=a_dst, b=b)

def bsli(a_dst: Reg, b: int | Absolute):
    """Stores `a_dst << b[0:5]` to `a_dst`"""
    return InsR4I5(0b1101_0000, r=a_dst, imm=Immediate(b, False, 0))

def bsri_tlsb(a_dst: Reg, b: int | Absolute):
    """Stores `a_dst >> b[0:5]` to `a_dst`. Stores the least significant bit of
    the result to the `T`-bit register"""
    return InsR4I5(0b1101_0010, r=a_dst, imm=Immediate(b, False, 0))

def basri(a_dst: Reg, b: int | Absolute):
    """Stores `a_dst >>> b[0:5]` to `a_dst`"""
    return InsR4I5(0b1101_0100, r=a_dst, imm=Immediate(b, False, 0))

def intoff():
    """Disables interrupts"""
    return RawInsU16(0b1110_0000_0000_0000)

def inton():
    """Enables interrupts"""
    return RawInsU16(0b1110_0001_0000_0000)

def intret():
    """Return from interrupt service routine. Re-enables interrupts and sets
    `rip` to the value stored in the special `rintret` register"""
    return RawInsU16(0b1110_0010_0000_0000)

def intwait():
    """Waits for an interrupt. Potentially puts the system in a sleep state. If
    interrupts are disabled, the core is hung."""
    return RawInsU16(0b1110_0011_0000_0000)
