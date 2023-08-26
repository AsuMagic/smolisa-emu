import abc
from dataclasses import dataclass, fields
from enum import Enum
from typing import Callable

from .register import *
from .label import *
from .util import *

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

@dataclass
class Align:
    to: int

@dataclass
class LinkTimeExpression:
    expr: Callable[[int, "Asm"], bytes]
    expanded_len: int

def is_access_valid(reg: Reg, access_method: RegAccessMode) -> bool:
    if access_method == RegAccessMode.R2:
        return 0b0000 <= reg.id <= 0b0011

    if access_method == RegAccessMode.Rh2:
        return 0b1000 <= reg.id <= 0b1011

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
        assert isinstance(self.value, int)

        ret = self.value >> self.shift

        if bits is not None:
            mask = (1 << bits) - 1
            ret &= mask

        return ret

def check_imm(imm: Immediate, encoding_bits: int):
    if not imm.is_materialized():
        return

    # already checked above but the linter doesn't care
    assert isinstance(imm.value, int)

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

        for field in fields(type(self)): # type: ignore
            attr = getattr(self, field.name)
            if isinstance(attr, Immediate) and not attr.is_materialized():
                assert patched_count == 0, "Cannot patch multiple immediates"
                attr.value = imm_lookup_callback(attr)
                patched_count += 1

    def check(self):
        pass

    def __post_init__(self):
        self.check()
