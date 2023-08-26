from enum import Enum
from dataclasses import dataclass

class SymbolVisibility(Enum):
    PUBLIC = 0
    PRIVATE = 1

@dataclass
class Label:
    name: str
    visibility: SymbolVisibility = SymbolVisibility.PRIVATE

    def __init__(self, name):
        self.name = name