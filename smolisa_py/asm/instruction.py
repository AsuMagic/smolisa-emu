from .register import *
from .label import *

class Instruction:
    pass

class InstructionI(Instruction):
    def __init__(self, ins, r1, imm8):
        assert isinstance(ins, int)
        assert isinstance(r1, Register)
        assert isinstance(imm8, (int, LabelAccess))

        self.ins = ins
        self.r1 = r1
        self.imm8 = imm8

class InstructionR(Instruction):
    def __init__(self, ins, r1, r2, r3=RG0):
        assert isinstance(ins, int)
        assert isinstance(r1, Register)
        assert isinstance(r2, Register)
        assert isinstance(r3, Register)

        self.ins = ins
        self.r1 = r1
        self.r2 = r2
        self.r3 = r3

def LI(dst, imm8):
    return InstructionI(0b0000, dst, imm8)

def LIU(dst, imm8):
    return InstructionI(0b0001, dst, imm8)

def LB(addr, dst):
    return InstructionR(0b0010, addr, dst)

def SB(addr, src):
    return InstructionR(0b0011, addr, src)

def LW(addr, dst):
    return InstructionR(0b0100, addr, dst)

def SW(addr, src):
    return InstructionR(0b0101, addr, src)

def LRZ(dst, src, cond):
    """`if (cond == 0) { dst = src }`"""
    return InstructionR(0b0110, dst, src, cond)

def LRNZ(dst, src, cond):
    """`if (cond != 0) { dst = src }`"""
    return InstructionR(0b0111, dst, src, cond)

def ADD(dst, lhs, rhs):
    return InstructionR(0b1000, dst, lhs, rhs)

def SUB(dst, lhs, rhs):
    return InstructionR(0b1001, dst, lhs, rhs)

def AND(dst, lhs, rhs):
    return InstructionR(0b1010, dst, lhs, rhs)

def OR(dst, lhs, rhs):
    return InstructionR(0b1011, dst, lhs, rhs)

def XOR(dst, lhs, rhs):
    return InstructionR(0b1100, dst, lhs, rhs)

def SHL(dst, lhs, rhs):
    return InstructionR(0b1101, dst, lhs, rhs)

def SHR(dst, lhs, rhs):
    return InstructionR(0b1110, dst, lhs, rhs)

def SWB(dst, lhs, rhs):
    return InstructionR(0b1111, dst, lhs, rhs)