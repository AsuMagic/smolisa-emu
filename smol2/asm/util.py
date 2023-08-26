def bytes_u16(value: int):
    return value.to_bytes(2, byteorder="little")

def bytes_u32(value: int):
    return value.to_bytes(4, byteorder="little")

def required_alignment(offset: int, to: int):
    if offset % to == 0:
        return 0

    return to - offset % to