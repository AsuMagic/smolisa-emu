import abc
from dataclasses import dataclass, fields
from enum import Enum
from functools import partial
from typing import Any, Union

from .register import *
from .label import *

class RegAccessMode(Enum):
    """Enumerates different encoding types for registers. Not every instruction
    can refer to any register due to encoding size constraints.
    
    Whether the register is a read or a write is not encoded by this, so R4
    will be used for R4, W4 and A4 access types in the docs/hardware."""
    R4 = 0
    R2 = 2
    Rh2 = 3

Relative = str

@dataclass
class Absolute:
    name: str

def is_access_valid(reg: Reg, access_method: RegAccessMode) -> bool:
    if access_method == RegAccessMode.R2:
        return 0b0000 <= reg.id <= 0b1111

    if access_method == RegAccessMode.Rh2:
        return 0b1000_0000 <= reg.id <= 0b1000_1111

    return True

def check_access(reg: Reg, access_method: RegAccessMode):
    if not is_access_valid(reg, access_method):
        raise ValueError(
            f"Register {reg} cannot be accessed with method {access_method}"
        )

@dataclass
class Immediate:
    value: int | Relative | Absolute
    signed: int
    shift: int = 0

    def is_materialized(self):
        return isinstance(self.value, int)

    def unscaled(self, bits=None):
        ret = self.value >> self.shift

        if bits is not None:
            mask = (1 << bits) - 1
            ret &= mask

        return ret

def check_imm(imm: Immediate, encoding_bits: int):
    if not imm.is_materialized():
        return

    if imm.signed:
        min_value = (-(1 << (encoding_bits - 1))) << imm.shift
        max_value = ((1 << (encoding_bits - 1)) - 1) << imm.shift
    else:
        min_value = 0
        max_value = ((1 << encoding_bits) - 1) << imm.shift

    if not (min_value <= imm.value <= max_value):
        raise ValueError(
            f"Immediate `{imm.value}` is out of range [{min_value}; {max_value}]"
        )

    if (imm.unscaled() << imm.shift) != imm.value:
        raise ValueError(
            f"Immediate `{imm.value}` is not a multiple of {1 << imm.shift}"
        )


def bytes_u16(value: int):
    return value.to_bytes(2, byteorder="little")

def bytes_u32(value: int):
    return value.to_bytes(4, byteorder="little")

class Instruction:
    def as_bytes(self):
        self_len = len(self)
        if self_len == 2:
            return bytes_u16(self.as_int())
        elif self_len == 4:
            return bytes_u32(self.as_int())
        else:
            assert False, "Unsupported instruction length"

    @abc.abstractmethod
    def as_int(self) -> int:
        pass

    @abc.abstractmethod
    def __len__(self) -> int:
        pass

    def patch_references(self, imm_lookup_callback):
        patched_count = 0

        for field in fields(type(self)):
            attr = getattr(self, field.name)
            if isinstance(attr, Immediate) and not attr.is_materialized():
                assert patched_count == 0, "Cannot patch multiple immediates"
                attr.value = imm_lookup_callback(attr)
                patched_count += 1

    def check(self):
        pass

    def __post_init__(self):
        self.check()

@dataclass
class RawInsU16(Instruction):
    raw: int

    def __len__(self):
        return 2

    def as_int(self):
        return self.raw

@dataclass
class InsR4R4(Instruction):
    op: int
    a: Reg
    b: Reg

    def check(self):
        check_access(self.a, RegAccessMode.R4)
        check_access(self.b, RegAccessMode.R4)

    def __len__(self):
        return 2

    def as_int(self):
        return (
            self.a.id
            | (self.b.id << 4)
            | (self.op << 8)
        )

@dataclass
class InsR4(Instruction):
    op: int
    r: Reg

    def __len__(self):
        return 2

    def check(self):
        check_access(self.r, RegAccessMode.R4)

    def as_int(self):
        return (
            self.r.id
            | (self.op << 8)
        )

@dataclass
class InsR4I4(Instruction):
    op: int
    r: Reg
    imm: Immediate

    def __len__(self):
        return 2

    def check(self):
        check_access(self.r, RegAccessMode.R4)
        check_imm(self.imm, 4)

    def as_int(self):
        return (
            self.r.id
            | (self.imm.unscaled(4) << 4)
            | (self.op << 8)
        )

@dataclass
class InsR4R4E16(Instruction):
    op: int
    a: Reg
    b: Reg
    imm: Immediate

    def __len__(self):
        return 4

    def check(self):
        check_access(self.a, RegAccessMode.R4)
        check_access(self.b, RegAccessMode.R4)
        check_imm(self.imm, 16)

    def as_int(self):
        return (
            self.a.id
            | (self.b.id << 4)
            | (self.op << 8)
            | (self.imm.unscaled(16) << 16)
        )

@dataclass
class InsRh2R2I6(Instruction):
    op: int
    a: Reg
    b: Reg
    imm: Immediate

    def __len__(self):
        return 2

    def check(self):
        check_access(self.a, RegAccessMode.Rh2)
        check_access(self.b, RegAccessMode.R2)
        check_imm(self.imm, 6)

    def as_int(self):
        return (
            (self.a.id & 0b11)
            | ((self.b.id & 0b11) << 2)
            | (self.imm.unscaled(6) << 4)
            | (self.op << 8)
        )

@dataclass
class InsR4I8(Instruction):
    op: int
    r: Reg
    imm: Immediate

    def __len__(self):
        return 2

    def check(self):
        check_access(self.r, RegAccessMode.R4)
        check_imm(self.imm, 8)

    def as_int(self):
        return (
            self.r.id
            | (self.imm.unscaled(8) << 4)
            | (self.op << 8)
        )


@dataclass
class InsR4I8E16(Instruction):
    op: int
    r: Reg
    imm: Immediate

    def __len__(self):
        return 4

    def check(self):
        check_access(self.r, RegAccessMode.R4)
        check_imm(self.imm, 24)

    def as_int(self):
        imm = self.imm.unscaled(24)
        imm_lower8 = imm & 0xFF
        imm_upper16 = imm >> 8
        return (
            self.r.id
            | (imm_lower8 << 4)
            | (self.op << 8)
            | (imm_upper16 << 16)
        )


@dataclass
class InsI12E16(Instruction):
    op: int
    imm: Immediate

    def __len__(self):
        return 4

    def check(self):
        check_imm(self.imm, 28)

    def as_int(self):
        imm = self.imm.unscaled(28)
        imm_lower12 = imm & 0xFFF
        imm_upper16 = imm >> 12
        return (
            imm_lower12
            | (self.op << 8)
            | (imm_upper16 << 16)
        )

@dataclass
class InsI12(Instruction):
    op: int
    imm: Immediate

    def __len__(self):
        return 2

    def check(self):
        check_imm(self.imm, 12)

    def as_int(self):
        return (
            self.imm.unscaled(12)
            | (self.op << 8)
        )

def l8(addr: Reg, dst: Reg):
    return InsR4R4(0b0000_0000, a=addr, b=dst)

def l16(addr: Reg, dst: Reg):
    return InsR4R4(0b0000_0001, a=addr, b=dst)

def l32(addr: Reg, dst: Reg):
    return InsR4R4(0b0000_0010, a=addr, b=dst)

def c_lr(src: Reg, dst: Reg):
    return InsR4R4(0b0000_0011, a=src, b=dst)

def l8ow(base: Reg, dst: Reg, off: int | Absolute):
    return InsR4R4E16(0b0000_0100, a=base, b=dst, imm=Immediate(off, signed=True, shift=0))

def l16ow(base: Reg, dst: Reg, off: int | Absolute):
    return InsR4R4E16(0b0000_0101, a=base, b=dst, imm=Immediate(off, signed=True, shift=1))

def l32ow(base: Reg, dst: Reg, off: int | Absolute):
    return InsR4R4E16(0b0000_0110, a=base, b=dst, imm=Immediate(off, signed=True, shift=2))

def lr(src: Reg, dst: Reg):
    return InsR4R4(0b0000_0111, a=src, b=dst)

def ls8(addr: Reg, dst: Reg):
    return InsR4R4(0b0000_1000, a=addr, b=dst)

def ls16(addr: Reg, dst: Reg):
    return InsR4R4(0b0000_1001, a=addr, b=dst)

def ls8ow(base: Reg, dst: Reg, off: int | Absolute):
    return InsR4R4E16(0b0000_0100, a=base, b=dst, imm=Immediate(off, signed=True, shift=0))

def ls16ow(base: Reg, dst: Reg, off: int | Absolute):
    return InsR4R4E16(0b0000_0101, a=base, b=dst, imm=Immediate(off, signed=True, shift=1))

def l8o(base: Reg, dst: Reg, off: int | Absolute):
    return InsRh2R2I6(0b0000_1100, a=base, b=dst, imm=Immediate(off, signed=False, shift=0))

def l16o(base: Reg, dst: Reg, off: int | Absolute):
    return InsRh2R2I6(0b0001_0000, a=base, b=dst, imm=Immediate(off, signed=False, shift=1))

def l32o(base: Reg, dst: Reg, off: int | Absolute):
    return InsRh2R2I6(0b0001_0100, a=base, b=dst, imm=Immediate(off, signed=False, shift=2))

def ls8o(base: Reg, dst: Reg, off: int | Absolute):
    return InsRh2R2I6(0b0001_1000, a=base, b=dst, imm=Immediate(off, signed=False, shift=0))

def ls16o(base: Reg, dst: Reg, off: int | Absolute):
    return InsRh2R2I6(0b0001_1100, a=base, b=dst, imm=Immediate(off, signed=False, shift=1))

def lsi(dst: Reg, imm: int | Absolute):
    return InsR4I8(0b0010_0000, r=dst, imm=Immediate(imm, signed=True))

def lsih(dst: Reg, imm: int | Absolute):
    return InsR4I8(0b0011_0000, r=dst, imm=Immediate(imm, signed=True))

def lsiw(dst: Reg, imm: int | Absolute):
    return InsR4I8E16(0b01000_0000, r=dst, imm=Immediate(imm, signed=True))

def liprel(dst: Reg, imm: int | Relative):
    return InsR4I8E16(0b0101_0000, r=dst, imm=Immediate(imm, signed=True))

def s8(addr: Reg, src: Reg):
    return InsR4R4(0b0110_0000, a=addr, b=src)

def s16(addr: Reg, src: Reg):
    return InsR4R4(0b0110_0001, a=addr, b=src)

def s32(addr: Reg, src: Reg):
    return InsR4R4(0b0110_0010, a=addr, b=src)

def push(src: Reg):
    return InsR4(0b0110_0011, r=src)

def s8ow(base: Reg, src: Reg, imm: int | Absolute):
    return InsR4R4E16(0b0110_0100, a=base, b=src, imm=Immediate(imm, signed=True, shift=0))

def s16ow(base: Reg, src: Reg, imm: int | Absolute):
    return InsR4R4E16(0b0110_0101, a=base, b=src, imm=Immediate(imm, signed=True, shift=1))

def s32ow(base: Reg, src: Reg, imm: int | Absolute):
    return InsR4R4E16(0b0110_0110, a=base, b=src, imm=Immediate(imm, signed=True, shift=2))

def brk():
    return RawInsU16(0b0110_0111_0000_0000)

def s8o(base: Reg, src: Reg, imm: int | Absolute):
    return InsRh2R2I6(0b0110_1000, a=base, b=src, imm=Immediate(imm, signed=False, shift=0))

def s16o(base: Reg, src: Reg, imm: int | Absolute):
    return InsRh2R2I6(0b0110_1100, a=base, b=src, imm=Immediate(imm, signed=False, shift=1))

def s32o(base: Reg, src: Reg, imm: int | Absolute):
    return InsRh2R2I6(0b0111_0000, a=base, b=src, imm=Immediate(imm, signed=False, shift=2))

def tltu(a: Reg, b: Reg):
    return InsR4R4(0b0111_0100, a, b)

def tlts(a: Reg, b: Reg):
    return InsR4R4(0b0111_0101, a, b)

def tgeu(a: Reg, b: Reg):
    return InsR4R4(0b0111_0110, a, b)

def tges(a: Reg, b: Reg):
    return InsR4R4(0b0111_0111, a, b)

def te(a: Reg, b: Reg):
    return InsR4R4(0b0111_1000, a, b)

def tne(a: Reg, b: Reg):
    return InsR4R4(0b0111_1001, a, b)

def tltsi(a: Reg, b_imm: int | Absolute):
    return InsR4I4(0b0111_1010, a, imm=Immediate(b_imm, signed=True))

def tgesi(a: Reg, b_imm: int | Absolute):
    return InsR4I4(0b0111_1011, a, imm=Immediate(b_imm, signed=True))

def tei(a: Reg, b_imm: int | Absolute):
    return InsR4I4(0b0111_1100, a, imm=Immediate(b_imm, signed=True))

def tnei(a: Reg, b_imm: int | Absolute):
    return InsR4I4(0b0111_1101, a, imm=Immediate(b_imm, signed=True))

def tbz(a: Reg):
    return InsR4(0b0111_1110, a)

def pl_l32(dst: Reg, off: int | Absolute):
    return InsR4I8(0b1000_0000, dst, imm=Immediate(off, signed=False, shift=2))

def j(addr: Reg):
    return InsR4(0b1001_0000, addr)

def c_j(addr: Reg):
    return InsR4(0b1001_0001, addr)

def jal(addr: Reg, target: Reg):
    return InsR4R4(0b1001_0010, a=addr, b=target)

def jali(imm: int | Relative):
    return InsI12E16(0b1010_0000, imm=Immediate(imm, signed=True, shift=1))

def c_ji(imm: int | Relative):
    return InsI12(0b1011_0000, imm=Immediate(imm, signed=True, shift=1))

def bsext8(dst: Reg, a: Reg):
    return InsR4R4(0b1100_0000, a=dst, b=a)

def bsext16(dst: Reg, a: Reg):
    return InsR4R4(0b1100_0001, a=dst, b=a)

def bzext8(dst: Reg, a: Reg):
    return InsR4R4(0b1100_0010, a=dst, b=a)

def bzext16(dst: Reg, a: Reg):
    return InsR4R4(0b1100_0011, a=dst, b=a)

def ineg(dst: Reg, a: Reg):
    return InsR4R4(0b1100_0100, a=dst, b=a)

def isub(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_0101, a=a_dst, b=b)

def iadd(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_0110, a=a_dst, b=b)

def iaddsi(a_dst: Reg, b: int | Absolute):
    return InsR4I4(0b1100_0111, r=a_dst, imm=Immediate(b, True, 0))

def iaddsiw(dst: Reg, a: Reg, b: int | Absolute):
    return InsR4R4E16(0b1100_1000, a=dst, b=a, imm=Immediate(b, True, 0))

def iaddsi_tnz(a_dst: Reg, b: int | Absolute):
    return InsR4I4(0b1100_1001, r=a_dst, imm=Immediate(b, True, 0))

def band(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_1010, a=a_dst, b=b)

def bor(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_1011, a=a_dst, b=b)

def bxor(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_1100, a=a_dst, b=b)

def bsl(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_1101, a=a_dst, b=b)

def bsr(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_1110, a=a_dst, b=b)

def basr(a_dst: Reg, b: Reg):
    return InsR4R4(0b1100_1111, a=a_dst, b=b)
