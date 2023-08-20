from .entities import *
from .label import *
from .register import *

import sys

def required_alignment(offset: int, to: int):
    if offset % to == 0:
        return 0

    return to - offset % to

class Asm:
    def __init__(self):
        self.sequences = []
        self.labels = {}

    def at(self, rom_address, contents):
        self.sequences.append((rom_address, contents))
    
    def set_label(self, address, name):
        if name in self.labels:
            raise ValueError(
                f"Label `{name}` was already defined: {self.labels[name]}"
            )
        self.labels[name] = address

    def resolve_label(self, name) -> int:
        if name not in self.labels:
            raise ValueError(
                f"Could not find label {name}"
            )

        return self.labels[name]

    def serialize_token(self, token, offset):
        if isinstance(token, Instruction):
            return token.as_bytes()
        elif isinstance(token, bytes):
            return token
        elif isinstance(token, Align):
            return bytes([0] * required_alignment(offset, token.to))
        else:
            assert False

    def token_len(self, token, offset):
        if isinstance(token, Instruction):
            return len(token)
        elif isinstance(token, bytes):
            return len(token)
        elif isinstance(token, Label):
            return 0
        elif isinstance(token, Absolute):
            return 4
        elif isinstance(token, Align):
            return required_alignment(offset, token.to)
        else:
            assert False

    def resolve_patch(self, ins_address, imm):
        if isinstance(imm.value, Absolute):
            return self.resolve_label(imm.value.name)
        elif isinstance(imm.value, Relative):
            relative_source = ins_address + 2
            relative_jump = self.resolve_label(imm.value) - relative_source
            return relative_jump

        assert False, f"Immediate type {type(imm)} failed to be resolved"

    def as_bytes(self, size):
        rom = bytearray([0x00 for i in range(size)])

        # TODO: detect sequence overlaps

        self._find_labels()
        
        for seq_offset, seq in self.sequences:
            offset = seq_offset

            for token in seq:
                if isinstance(token, Label):
                    continue

                if isinstance(token, Instruction):
                    token.patch_references(lambda imm: \
                        self.resolve_patch(offset, imm)
                    )
                    token.check()

                if isinstance(token, Absolute):
                    token = self.resolve_label(token.name).to_bytes(4, byteorder="little")

                token_bytes = self.serialize_token(token, offset)
                assert token_bytes is not None, f"Erroneous token of type {type(token)}"
                assert len(token_bytes) == self.token_len(token, offset)

                if offset + len(token_bytes) > len(rom):
                    raise ValueError(f"ROM file too small for sequence of size {len(token_bytes)} at offset 0x{offset:04X}")

                rom[offset:offset + len(token_bytes)] = token_bytes
                offset += len(token_bytes)
        
        return rom

    def to_rom(self, rom_size=65536):
        sys.stdout.buffer.write(self.as_bytes(rom_size))

    def _find_labels(self):
        for seq_offset, seq in self.sequences:
            offset = seq_offset

            for token in seq:
                if isinstance(token, Label):
                    self.set_label(offset, token.name)
                
                offset += self.token_len(token, offset)
