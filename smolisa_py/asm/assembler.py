from .instruction import *
from .label import *
from .register import *

import sys

def resolve_relative_address(ins_address, label_address):
    relative_source = ins_address + 2
    relative_jump = label_address - relative_source
    print(relative_jump)
    return relative_jump

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

    def serialize_token(self, token):
        if isinstance(token, Instruction):
            return token.as_bytes()
        elif isinstance(token, bytes):
            return token
        else:
            assert None

    def as_bytes(self, size):
        rom = bytearray([0x00 for i in range(size)])

        # TODO: detect sequence overlaps

        self._process_labels()
        
        for seq_offset, seq in self.sequences:
            offset = seq_offset

            for token in seq:
                if isinstance(token, Label):
                    continue

                if isinstance(token, Instruction):
                    token.check()

                token_bytes = self.serialize_token(token)
                assert token_bytes is not None, f"Erroneous token of type {type(token)}"

                if offset + len(token_bytes) > len(rom):
                    raise ValueError(f"ROM file too small for sequence of size {len(token_bytes)} at offset 0x{offset:04X}")

                rom[offset:offset + len(token_bytes)] = token_bytes
                offset += len(token_bytes)
        
        return rom

    def to_rom(self, rom_size=65536):
        sys.stdout.buffer.write(self.as_bytes(rom_size))

    def _process_labels(self):
        for seq_offset, seq in self.sequences:
            offset = seq_offset
            
            for token in seq:
                if isinstance(token, Label):
                    self.set_label(offset, token.name)
                elif isinstance(token, Instruction):
                    token.patch_references(lambda label: resolve_relative_address(offset, self.labels[label]))
                    offset += len(self.serialize_token(token))
                elif isinstance(token, bytes):
                    offset += len(token)