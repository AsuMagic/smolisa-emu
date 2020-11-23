class Register:
    def __init__(self, id):
        self.id = id

regs = [Register(i) for i in range(16)]
RG0, RG1, RG2, RG3, RG4, RG5, RG6, RG7, RG8, RG9, RG10, RG11, RG12, RG13, RIP, RBANK = regs
