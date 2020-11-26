from .instruction import *
from .label import *
from .register import *

import sys

class Asm:
    def __init__(self):
        self.sequences = []
        self.labels = {}

    def at(self, rom_address, contents):
        self.sequences.append((rom_address, contents))
    
    def set_label(self, address, name):
        assert name not in self.labels
        self.labels[name] = address
    
    def as_bytes(self, size):
        rom = bytearray([0x00 for i in range(size)])

        # TODO: detect sequence overlaps

        self._process_labels()
        
        for seq_offset, seq in self.sequences:
            offset = seq_offset

            for token in seq:
                token_length = self._token_length(token)

                if offset + token_length > len(rom):
                    raise ValueError(f"ROM file too small for sequence of size {token_length} at offset 0x{offset:04X}")

                if isinstance(token, Instruction):
                    opcode = self._opcode(token)
                    rom[offset]     = (opcode >> 0) & 0xFF
                    rom[offset + 1] = (opcode >> 8) & 0xFF
                elif isinstance(token, bytes):
                    rom[offset:offset+token_length] = token
                else:
                    assert isinstance(token, Label), "Unexpected token type"
                
                offset += self._token_length(token)
        
        return rom
    
    def as_int16_list(self, size_bytes):
        rom_bytes = self.as_bytes(size_bytes)
        return [
            rom_bytes[i * 2] | (rom_bytes[(i * 2) + 1] << 8)
            for i in range(size_bytes // 2)
        ]

    def to_rom(self, rom_size=65536):
        sys.stdout.buffer.write(self.as_bytes(rom_size))
    
    def _resolve_reg(self, operand):
        if isinstance(operand, Register):
            return operand.id
        
        raise TypeError(f"Not a register: {type(operand)}")
    
    def _resolve_imm8(self, operand):
        if isinstance(operand, int):
            if not 0x00 <= operand <= 0xFF:
                raise ValueError(f"Operand does not fit in byte: {operand}")
            
            return operand
        
        if isinstance(operand, LabelAccess):
            label_offset = self.labels[operand.name]
            
            if operand.lower_byte:
                return (label_offset >> 0) & 0xFF
            else:
                return (label_offset >> 8) & 0xFF

        raise TypeError(f"Not an integral immediate: {type(operand)}")
    
    def _opcode(self, ins):
        if isinstance(ins, InstructionI):
            return (
                (ins.ins << 0)
                | (self._resolve_reg(ins.r1) << 4)
                | (self._resolve_imm8(ins.imm8) << 8)
            )
        elif isinstance(ins, InstructionR):
            return (
                (ins.ins << 0)
                | (self._resolve_reg(ins.r1) << 4)
                | (self._resolve_reg(ins.r2) << 8)
                | (self._resolve_reg(ins.r3) << 12)
            )
        else:
            raise TypeError(f"Unexpected instruction type {type(ins)}")

    def _token_length(self, token):
        if isinstance(token, bytes):
            return len(token)
        
        if isinstance(token, Instruction):
            return 2
        
        if isinstance(token, Label):
            return 0
        
        raise TypeError(f"Unexpected token type {type(token)}")

    def _process_labels(self):
        for seq_offset, seq in self.sequences:
            offset = seq_offset
            
            for token in seq:
                if isinstance(token, Label):
                    self.set_label(offset, token.name)
                
                offset += self._token_length(token)