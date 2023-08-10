from smol2.asm.assembler import *
import smol2.asm.macros as m

# Untested; might be completely broken

asm = Asm()

tape_base = 0x00008000

reg_tape = r0
reg_ip = r1
reg_jt_base = r2
reg_depth = r3
reg_fallback_addr = r4
reg_cst_loopbegin = r5
reg_cst_loopend = r6

bf_source = "+[+.]+[]"

def make_jump_table():
    table = [Absolute("FallbackDispatch") for _ in range(128)]

    table[ord("+")] = Absolute("OpInc")
    table[ord("-")] = Absolute("OpDec")
    table[ord(">")] = Absolute("OpR")
    table[ord("<")] = Absolute("OpL")
    table[ord(".")] = Absolute("OpWrite")
    table[ord("[")] = Absolute("OpLoopStart")
    table[ord("]")] = Absolute("OpLoopEnd")

    return table

def dispatch_generic():
    return [
        # goto *(*ip + DispatchJT)
        l8(addr=reg_ip, dst=r12),
        iadd(a_dst=r12, b=reg_jt_base),
        l32(addr=r12, dst=r12),
        j(r12),
    ]

def inc_dispatch_generic():
    return [
        # ++ip
        iaddsi(reg_ip, 1),
        *dispatch_generic()
    ]


asm.at(0x0000_0000, [
    lsiw(reg_ip, Absolute("BFProgram")),
    lsiw(reg_tape, tape_base),
    lsiw(reg_fallback_addr, Absolute("FallbackDispatch")),

    lsiw(reg_cst_loopbegin, ord('[')),
    lsiw(reg_cst_loopend, ord(']')),

    lsiw(reg_jt_base, Absolute("DispatchJT")),

    *dispatch_generic(),

    Label("FallbackDispatch"),
        *inc_dispatch_generic(),

    Label("OpInc"),
        l8(addr=reg_tape, dst=r11),
        iaddsi(r11, 1),
        *inc_dispatch_generic(),

    Label("OpDec"),
        l8(addr=reg_tape, dst=r11),
        iaddsi(r11, -1),
        *inc_dispatch_generic(),

    Label("OpR"),
        iaddsi(reg_tape, 1),
        *inc_dispatch_generic(),
    
    Label("OpL"),
        iaddsi(reg_tape, -1),
        *inc_dispatch_generic(),

    Label("OpWrite"),
        brk(),
        *inc_dispatch_generic(),

    Label("OpLoopStart"),
        l8(addr=reg_tape, dst=r11),
        tnei(r11, 0), # *sp != 0? then we can enter the BF loop
        c_ji("FallbackDispatch"),

        Label("FindEndLoop"),
        # else loop til ] of same depth
        # trick: reg_depth == 0 at any starting point
        iaddsi(reg_ip, 1),

        l8(reg_ip, r10),
        te(r10, reg_cst_loopbegin), # *ip == '['? jump to handler
        c_ji("FindEndLoop:FoundLoopStart"),
        tne(r10, reg_cst_loopend), # *ip != ']'? loop again
        c_ji("FindEndLoop"),

        Label("FindEndLoop:FoundLoopEnd"),
            iaddsi_tnz(reg_depth, -1), # --depth
            c_ji("FindEndLoop"), # if depth != 0: loop back
            j(reg_fallback_addr), # depth == 0, thus break out, and let ip inc

        Label("FindEndLoop:FoundLoopStart"),
            iaddsi_tnz(reg_depth, 1), # ++depth
            c_ji("FindEndLoop"), # hack: no short ji yet so abuse tnz above (always true)

    Label("OpLoopEnd"),
        l8(addr=reg_tape, dst=r11),
        tei(r11, 0), # *sp == 0? then we can exit the BF loop
        c_ji("FallbackDispatch"),

        Label("FindStartLoop"),
        # else loop til [ of same depth
        iaddsi(reg_ip, -1),

        l8(reg_ip, r10),
        te(r10, reg_cst_loopbegin), # *ip == '['? jump to handler
        c_ji("FindStartLoop:FoundLoopStart"),
        tne(r10, reg_cst_loopend), # *ip != ']'? loop again
        c_ji("FindStartLoop"),

        Label("FindStartLoop:FoundLoopEnd"),
            iaddsi_tnz(reg_depth, 1), # ++depth
            c_ji("FindStartLoop"),

        Label("FindStartLoop:FoundLoopStart"),
            iaddsi_tnz(reg_depth, -1), # --depth
            c_ji("FindStartLoop"), # if depth != 0: loop back
            j(reg_fallback_addr), # depth == 0, thus break out, and let ip inc

    Label("DispatchJT"),
    *make_jump_table(),

    Label("BFProgram"),
    bf_source.encode("ascii"),
])

if __name__ == "__main__":
    asm.to_rom()
