#pragma once

#include <smol/common/masks.hpp>
#include <smol/common/registers.hpp>
#include <smol/common/types.hpp>

#include <array>

namespace formats
{
struct TypeI
{
	RegisterId r;
	Byte       imm8;

	TypeI(Word instruction) :
		r{RegisterId((instruction >> 4) & masks::reg)}, imm8{Byte((instruction >> 8) & masks::imm)}
	{}
};

struct TypeR
{
	RegisterId r1, r2, r3;

	TypeR(Word instruction) :
		r1{RegisterId((instruction >> 4) & masks::reg)},
		r2{RegisterId((instruction >> 8) & masks::reg)},
		r3{RegisterId((instruction >> 12) & masks::reg)}
	{}
};

} // namespace formats
