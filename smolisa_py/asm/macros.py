from .instruction import *

def load_register(dst, src):
    return [OR(dst, src, src)]

def _validate_imm16_write_to(dst):
    assert dst not in (RBANK, RIP), "cannot do atomic imm16 write to registers, you will need a temporary register for expected behavior on $bank and $ip"

def load_imm16(dst, imm16):
    _validate_imm16_write_to(dst)

    if imm16 == 0:
        return [
            XOR(dst, dst, dst)
        ]
    
    return [
        LIU(dst, (imm16 >> 8) & 0xFF),
        LI(dst, imm16 & 0xFF)
    ]

def load_imm16_assume_zero(dst, imm16):
    _validate_imm16_write_to(dst)

    if imm16 < 256:
        return [
            LI(dst, imm16)
        ]
    
    if imm16 == 0:
        return []
    
    return load_imm16(dst, imm16)

def load_label(dst, label):
    _validate_imm16_write_to(dst)

    return [
        LIU(dst, high(label)),
        LI(dst, low(label))
    ]
