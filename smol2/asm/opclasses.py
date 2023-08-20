from .entities import *

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
class InsR4I5(Instruction):
    op: int
    r: Reg
    imm: Immediate

    def __len__(self):
        return 2

    def check(self):
        check_access(self.r, RegAccessMode.R4)
        check_imm(self.imm, 5)

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