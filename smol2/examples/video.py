from smol2.asm import *

asm = Asm()

palette_white = 0x0011

reg_rom_ptr = r0
reg_fb_stop_address = r1
reg_fb_ptr = r8
reg_tmp = r2
reg_pal_white = r4
reg_char_group = r12
reg_vsync_address = r6
reg_single_bit = r11
# reg_palette_white = RG11

video_path = "./assets/badapple.bin"

with open(video_path, "rb") as f:
    video_bytes = f.read()

num_frames = len(video_bytes) // (80 * 25)
wait_for = 1 * 30 # frames

asm.at(0x0000, [
    liprel(rpl, "Literals"),

    pl_l32(reg_fb_stop_address, 8),
    pl_l32(reg_vsync_address, 4),

    # print some metadata for absolutely no reason other than it looks cool
    # the registers used will be overwritten after, we don't care about them
    pl_l32(reg_fb_ptr, 3*4),
    liprel(dst=reg_rom_ptr, imm="PrintedMeta1"),

    Label("MetaPrintLoop"),
    l8(addr=reg_rom_ptr, dst=reg_tmp),
    tnei(reg_tmp, 0),
    s8(addr=reg_fb_ptr, src=reg_tmp),
    iaddsi(reg_rom_ptr, 1),
    iaddsi(reg_fb_ptr, 2),
    c_ji("MetaPrintLoop"),

    # wait for `wait_for` frames before starting the video
    lsi(reg_tmp, wait_for),
    Label("VSyncDelayLoop"),
    s8(addr=reg_vsync_address, src=r0), # src irrelevant
    iaddsi_tnz(reg_tmp, -1),
    c_ji("VSyncDelayLoop"),

    # begin rendering video
    liprel(dst=reg_rom_ptr, imm="Video"),
    lsi(dst=reg_pal_white, imm=palette_white),

    # while(true)
    Label("prepareframe"),
        pl_l32(reg_fb_ptr, 0),

        # while (framebuffer_address != framebuffer_stop_address)
        Label("drawframe"),
            l8(addr=reg_rom_ptr, dst=reg_char_group),

            # *framebuffer_address = is_set ? PALETTE_BLACK : PALETTE_WHITE
            # PALETTE_BLACK is equal to 0x0000
            bsri_tlsb(reg_char_group, 0),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8(addr=reg_fb_ptr, src=reg_tmp),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_ptr, src=reg_tmp, imm=2),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_ptr, src=reg_tmp, imm=4),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_ptr, src=reg_tmp, imm=6),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_ptr, src=reg_tmp, imm=8),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_ptr, src=reg_tmp, imm=10),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_ptr, src=reg_tmp, imm=12),

            bsri_tlsb(reg_char_group, 1),
            lsi(dst=reg_tmp, imm=0),
            c_lr(src=reg_pal_white, dst=reg_tmp),
            s8o(base=reg_fb_ptr, src=reg_tmp, imm=14),

            iaddsiw(reg_fb_ptr, reg_fb_ptr, 16),
            iaddsi(reg_rom_ptr, 1),

            # (handle looping)
            # while (framebuffer_address != framebuffer_stop_address)
            tne(reg_fb_ptr, reg_fb_stop_address),
            c_ji("drawframe"),

            # fallthrough
        
        # wait_vsync()
        s8(addr=reg_vsync_address, src=r0), # doesn't matter what we write

        # (handle looping)
        # while (true)
        jali("prepareframe"),

    Align(4),
    Label("Literals"),
    (0xF0002001).to_bytes(4, byteorder="little"), # [0] fb start (color)
    (0xF0002FD0).to_bytes(4, byteorder="little"), # [1] vsync
    (0xF0002FA1).to_bytes(4, byteorder="little"), # [2] fb end
    (0xF0002000).to_bytes(4, byteorder="little"), # [3] fb start

    Label("PrintedMeta1"),
    f"Assembled from {video_path} ({num_frames} frames)".encode("utf-8"),

    Align(4),
    Label("Video"),
    video_bytes
])

if __name__ == "__main__":
    asm.to_rom(2048 * 1024)