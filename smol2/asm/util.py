

def bytes_u16(value: int):
    return value.to_bytes(2, byteorder="little")

def bytes_u32(value: int):
    return value.to_bytes(4, byteorder="little")