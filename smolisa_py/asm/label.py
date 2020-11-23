class Label:
    def __init__(self, name):
        self.name = name

class LabelAccess:
    def __init__(self, name, lower_byte):
        self.name = name
        self.lower_byte = lower_byte

def high(name):
    return LabelAccess(name, False)

def low(name):
    return LabelAccess(name, True)