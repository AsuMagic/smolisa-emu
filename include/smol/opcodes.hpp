#pragma once

enum class Opcode
{
	Li   = 0b0000,
	Liu  = 0b0001,
	Lb   = 0b0010,
	Sb   = 0b0011,
	Lw   = 0b0100,
	Sw   = 0b0101,
	Lrz  = 0b0110,
	Lrnz = 0b0111,
	Add  = 0b1000,
	Sub  = 0b1001,
	And  = 0b1010,
	Or   = 0b1011,
	Xor  = 0b1100,
	Shl  = 0b1101,
	Shr  = 0b1110,
	Swb  = 0b1111,
};
