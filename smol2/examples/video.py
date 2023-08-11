from smol2.asm.assembler import *
import smol2.asm.macros as macros

asm = Asm()

palette_white = 0x0011

reg_rom_ptr = r0
reg_fb_stop_address = r1
reg_fb_address = r2
reg_cst_mask_1 = r3
reg_cst_palette_white = r4
reg_char_group = r12
reg_char_group_stop_address = r5
reg_vsync_address = r6
reg_single_bit = r11
# reg_palette_white = RG11

asm.at(0x0000, [
    liprel(rpl, "Literals"),

    lsi(dst=reg_rom_ptr, imm=Absolute("Video")),
    lsi(dst=reg_cst_mask_1, imm=1),
    lsi(dst=reg_cst_palette_white, imm=palette_white),

    pl_l32(reg_fb_stop_address, 8),
    pl_l32(reg_vsync_address, 4),

    # while(true)
    Label("prepareframe"),
        pl_l32(reg_fb_address, 0),

        # while (framebuffer_address != framebuffer_stop_address)
        Label("drawframe"),
            l8(addr=reg_rom_ptr, dst=reg_char_group),

            # const char* char_group_stop_address = framebuffer_address + (8 * 2);
            iaddsiw(dst=reg_char_group_stop_address, a=reg_fb_address, b=16),

            # while (framebuffer_address != char_group_stop_address)
            Label("drawpixelgroup"),

                # bool is_set = (char_group & 0x01) != 0;
                lr(src=reg_char_group, dst=reg_single_bit),
                band(a_dst=reg_single_bit, b=reg_cst_mask_1),

                # *framebuffer_address = is_set ? PALETTE_BLACK : PALETTE_WHITE
                # PALETTE_BLACK is equal to 0x0000
                lsi(dst=r9, imm=0),
                tei(reg_single_bit, 0),
                c_lr(src=reg_cst_palette_white, dst=r9),
                s8(addr=reg_fb_address, src=r9),

                # char_group = char_group >> 1;
                bsri(reg_char_group, 1),

                # framebuffer_address += 2;
                iaddsi(reg_fb_address, 2),

                # (handle looping)
                tne(reg_fb_address, reg_char_group_stop_address),
                brk(),
                c_ji("drawpixelgroup"),

                # fallthrough

            iaddsi(reg_rom_ptr, 1),
            brk(),

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

    brk(), # FIXME: Align(2)
    Label("Literals"),
    (0xF0002001).to_bytes(4, byteorder="little"), # [0] fb start
    (0xF0002FD0).to_bytes(4, byteorder="little"), # [1] vsync
    (0xF0002FA1).to_bytes(4, byteorder="little"), # [2] fb end

    Label("Video"),
    open("./assets/badapple.bin", "rb").read()
])

if __name__ == "__main__":
    asm.to_rom(2048 * 1024)