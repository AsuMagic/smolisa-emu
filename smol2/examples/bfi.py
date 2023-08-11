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
reg_cst_fb_vsync = r7
reg_fb_off = r8
reg_cst_fb_lastoff = r9

bf_source = """
->++>+++>+>+>+++>>>>>>>>>>>>>>>>>>>>+>+>++>+++>++>>+++>+>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>+>+>>+++>>+++>>>>>+++>+>>>>>>>>>++>+++>+++>+>>+++>>>+++>+>++>+++>>>+>+>
++>+++>+>+>>+++>>>>>>>+>+>>>+>+>++>+++>+++>+>>+++>>>+++>+>++>+++>++>>+>+>++>+++>
+>+>>+++>>>>>+++>+>>>>>++>+++>+++>+>>+++>>>+++>+>+++>+>>+++>>+++>>++[[>>+[>]++>+
+[<]<-]>+[>]<+<+++[<]<+]>+[>]++++>++[[<++++++++++++++++>-]<+++++++++.<]
+[]
"""

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
        bsli(a_dst=r12, b=2),
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
    liprel(rpl, "Literals"),
    lsiw(reg_ip, Absolute("BFProgram")),
    lsiw(reg_tape, tape_base),
    lsiw(reg_fallback_addr, Absolute("FallbackDispatch")),

    lsiw(reg_cst_loopbegin, ord('[')),
    lsiw(reg_cst_loopend, ord(']')),

    pl_l32(reg_fb_off, 0),
    pl_l32(reg_cst_fb_vsync, 4),
    pl_l32(reg_cst_fb_lastoff, 8),

    lsiw(reg_jt_base, Absolute("DispatchJT")),

    *dispatch_generic(),

    Label("FallbackDispatch"),
        *inc_dispatch_generic(),

    Label("OpInc"),
        l8(addr=reg_tape, dst=r11),
        iaddsi(r11, 1),
        s8(addr=reg_tape, src=r11),
        *inc_dispatch_generic(),

    Label("OpDec"),
        l8(addr=reg_tape, dst=r11),
        iaddsi(r11, -1),
        s8(addr=reg_tape, src=r11),
        *inc_dispatch_generic(),

    Label("OpR"),
        iaddsi(reg_tape, 1),
        *inc_dispatch_generic(),
    
    Label("OpL"),
        iaddsi(reg_tape, -1),
        *inc_dispatch_generic(),

    Label("OpWrite"),
        l8(addr=reg_tape, dst=r11),
        s8(addr=reg_fb_off, src=r11),
        s8(addr=reg_cst_fb_vsync, src=r0),
        iaddsi(reg_fb_off, 2),
        tltu(a=reg_fb_off, b=reg_cst_fb_lastoff),
        c_ji("FallbackDispatch"),

        # Wraparound logic
        pl_l32(reg_fb_off, 0),

        # brk(),
        *inc_dispatch_generic(),

    Label("OpLoopStart"),
        l8(addr=reg_tape, dst=r11),
        tnei(r11, 0), # *sp != 0? then we can enter the BF loop
        c_ji("FallbackDispatch"),

        lsi(reg_depth, 1),
        Label("FindEndLoop"),
        # else loop til ] of same depth
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

        lsi(reg_depth, 1),
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
])

asm.at(0x1000, [
    Label("Literals"),
    (0xF0002000).to_bytes(4, byteorder="little"), # [0]

    # 2FD0 hangs til vsync, 2FD1 only presents if it gets a chance
    (0xF0002FD1).to_bytes(4, byteorder="little"), # [1]

    (0xF0002F9F).to_bytes(4, byteorder="little"), # [2]
    Label("DispatchJT"),
    *make_jump_table(),
])

asm.at(0x3000, [
    Label("BFProgram"),
    bf_source.encode("ascii"),
])

if __name__ == "__main__":
    asm.to_rom()
