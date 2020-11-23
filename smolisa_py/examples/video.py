from smolisa_py.asm.assembler import *
import smolisa_py.asm.macros as macros

asm = Asm()

video_rom_start_address = 0x2000
video_rom_start_bank = 0x0001
fb_start_address = 0x2001
fb_stop_address = 0x2FA1
mmio_bank = 0xFFFF

reg_rom_ptr = RG0
reg_rom_bank = RG1
reg_fb_stop_address = RG2
reg_fb_address = RG3
reg_char_group = RG4
reg_char_group_stop_address = RG5
reg_is_set = RG6
reg_mmio_bank = RG7
reg_tmp = RG13
reg_tmp2 = RG12

asm.at(0x0000, [
    *macros.load_imm16(reg_rom_ptr, video_rom_start_address),
    *macros.load_imm16(reg_rom_bank, video_rom_start_bank),
    *macros.load_imm16(reg_mmio_bank, mmio_bank),
    *macros.load_imm16(reg_fb_stop_address, fb_stop_address),

    # while(true)
    Label("prepareframe"),
        *macros.load_imm16(reg_fb_address, fb_start_address),

        # while (framebuffer_address != framebuffer_stop_address)
        Label("drawframe"),
        
            # fetch char group from the data bank
            *macros.load_register(RBANK, reg_rom_bank),
            LB(addr=reg_rom_ptr, dst=reg_char_group),

            # move to the mmio bank
            *macros.load_register(RBANK, reg_mmio_bank),

            # const char* char_group_stop_address = framebuffer_address + (8 * 2);
            *macros.load_imm16(reg_tmp, 8 * 2),
            ADD(dst=reg_char_group_stop_address, lhs=reg_fb_address, rhs=reg_tmp),

            # while (framebuffer_address != char_group_stop_address)
            Label("drawpixelgroup"),

                # bool is_set = (char_group & 0x01) != 0;
                *macros.load_imm16(reg_tmp, 1),
                AND(dst=reg_is_set, lhs=reg_char_group, rhs=reg_tmp),

                # if (is_set)
                *macros.load_label(reg_tmp, "setwhite"),
                LRNZ(dst=RIP, src=reg_tmp, cond=reg_is_set),

                Label("setblack"),
                    # *framebuffer_address = PALETTE_BLACK;
                    XOR(reg_tmp, reg_tmp, reg_tmp),
                    SB(addr=reg_fb_address, src=reg_tmp),

                    # (handle branching)
                    *macros.load_label(reg_tmp, "setcolorend"),
                    *macros.load_register(dst=RIP, src=reg_tmp),
                
                Label("setwhite"),
                    # *framebuffer_address = PALETTE_BLACK;
                    *macros.load_imm16(reg_tmp, 0x0011),
                    SB(addr=reg_fb_address, src=reg_tmp),

                    # fallthrough
                
                Label("setcolorend"),
                # char_group = char_group >> 1;
                *macros.load_imm16(reg_tmp, 1),
                SHR(dst=reg_char_group, lhs=reg_char_group, rhs=reg_tmp),

                # framebuffer_address += 2;
                *macros.load_imm16(reg_tmp, 2),
                ADD(reg_fb_address, reg_fb_address, reg_tmp),

                # (handle looping)
                # while (framebuffer_address != char_group_stop_address)
                *macros.load_label(reg_tmp2, "drawpixelgroup"),
                SUB(dst=reg_tmp, lhs=reg_fb_address, rhs=reg_char_group_stop_address),
                LRNZ(dst=RIP, src=reg_tmp2, cond=reg_tmp),

                # fallthrough
            
            # rom_ptr += 1;
            *macros.load_imm16(reg_tmp, 1),
            ADD(reg_rom_ptr, reg_rom_ptr, reg_tmp),

            # if (rom_ptr == 0)
            # => if (rom_ptr != 0) skip
            *macros.load_label(reg_tmp, "skipbankswitch"),
            LRNZ(dst=RIP, src=reg_tmp, cond=reg_rom_ptr),

                *macros.load_imm16(reg_rom_ptr, video_rom_start_address),

                *macros.load_imm16(reg_tmp, 1),
                ADD(dst=reg_rom_bank, lhs=reg_rom_bank, rhs=reg_tmp),

                # fallthrough
            
            Label("skipbankswitch"),
                # (handle looping)
                # while (framebuffer_address != framebuffer_stop_address)
                *macros.load_label(reg_tmp2, "drawframe"),
                SUB(reg_tmp, reg_fb_address, reg_fb_stop_address),
                LRNZ(dst=RIP, src=reg_tmp2, cond=reg_tmp),

                # fallthrough
        
        # wait_vsync()
        *macros.load_imm16(reg_tmp, 0x2FD0),
        SB(addr=reg_tmp, src=reg_tmp), # doesn't matter what we write

        # (handle looping)
        # while (true)
        *macros.load_label(reg_tmp, "prepareframe"),
        *macros.load_register(dst=RIP, src=reg_tmp)
])

with open("./assets/badapple.bin", "rb") as file:
    asm.at(0x2000, [
        Label("videostart"),
        bytes(file.read())
    ])

asm.to_rom(2048 * 1024)