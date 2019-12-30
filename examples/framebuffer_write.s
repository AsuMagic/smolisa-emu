; Example: write 'a' to the top left of the framebuffer
;          note: this assumes an emulator and/or firmware that initializes:
;                - the palette;
;                - character color codes;
;                otherwise nothing will be shown on screen.

; TYPE-I                -r1- -op- ---imm8--
; TYPE-R                -r1- -op- -r3- -r2-

; set bank 0xFFFF
xor $bank $bank $bank ; 1111'1011 1111'1111
not $bank $bank       ; 1111'1000 xxxx'1111

; load address 0x2000 to $g0
; 0x20 = 32
li $g0 32           ; 0000'0000 0010'0000
swb $g0 $g0           ; 0000'1110 xxxx'xxxx
; 0x00 = 0
li $g0 0           ; 0000'0000 0000'0000

; load character 'a' to $g1
; we dont care about the upper byte
li $g1 'a'            ; 0001'0000 0110'0001

; copy 'a' to addr $g0
sm $g0 $g1            ; 0000'0010 xxxx'0001