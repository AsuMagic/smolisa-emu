from smol2.asm.assembler import *

asm = Asm()

palette_white = 0x0011

reg_rom_ptr = r0
reg_fb_stop_address = r1
reg_fb_address = r8
reg_tmp = r2
reg_pal_white = r4
reg_char_group = r12
reg_vsync_address = r6
reg_single_bit = r11
# reg_palette_white = RG11

asm.at(0x0000, [
    liprel(rpl, "Literals"),

    lsi(dst=reg_rom_ptr, imm=Absolute("Video")),
    lsi(dst=reg_pal_white, imm=palette_white),

    pl_l32(reg_fb_stop_address, 8),
    pl_l32(reg_vsync_address, 4),

    # while(true)
    Label("prepareframe"),
        pl_l32(reg_fb_address, 0),

        # while (framebuffer_address != framebuffer_stop_address)
        Label("drawframe"),
            l8(addr=reg_rom_ptr, dst=reg_char_group),

            # *framebuffer_address = is_set ? PALETTE_BLACK : PALETTE_WHITE
            # PALETTE_BLACK is equal to 0x0000
            bsri_tlsb(reg_char_group, 0),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8(addr=reg_fb_address, src=reg_tmp),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_address, src=reg_tmp, imm=2),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_address, src=reg_tmp, imm=4),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_address, src=reg_tmp, imm=6),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_address, src=reg_tmp, imm=8),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_address, src=reg_tmp, imm=10),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_address, src=reg_tmp, imm=12),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_address, src=reg_tmp, imm=14),

            iaddsiw(reg_fb_address, reg_fb_address, 16),
            iaddsi(reg_rom_ptr, 1),

            # (handle looping)
            # while (framebuffer_address != framebuffer_stop_address)
            tne(reg_fb_address, reg_fb_stop_address),
            c_ji("drawframe"),

            # fallthrough
        
        # wait_vsync()
        s8(addr=reg_vsync_address, src=r0), # doesn't matter what we write

        # (handle looping)
        # while (true)
        jali("prepareframe"),

    Align(4),
    Label("Literals"),
    (0xF0002001).to_bytes(4, byteorder="little"), # [0] fb start
    (0xF0002FD0).to_bytes(4, byteorder="little"), # [1] vsync
    (0xF0002FA1).to_bytes(4, byteorder="little"), # [2] fb end

    Label("Video"),
    open("./assets/badapple.bin", "rb").read()
])

if __name__ == "__main__":
    asm.to_rom(2048 * 1024)