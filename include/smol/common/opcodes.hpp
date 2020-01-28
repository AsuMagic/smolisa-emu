#pragma once

enum class Opcode
{
	Li  = 0b0000,
	Liu = 0b0001,
	Lm  = 0b0010,
	Sm  = 0b0011,
	Bz  = 0b0100,
	Bnz = 0b0101,
	Add = 0b0110,
	Sub = 0b0111,
	Not = 0b1000,
	And = 0b1001,
	Or  = 0b1010,
	Xor = 0b1011,
	Shl = 0b1100,
	Shr = 0b1101,
	Swb = 0b1110,
};
