#pragma once

#include <array>
#include <string_view>

enum class InstructionType
{
	R8R8R8,
	R8Imm8
};

struct InstructionInfo
{
	std::string_view mnemonic;
	std::size_t useful_parameters = 0;
	InstructionType type = InstructionType::R8Imm8;
};

static constexpr std::array<InstructionInfo, 16> instruction_infos {{
	{"li", 2, InstructionType::R8Imm8},
	{"lm", 2, InstructionType::R8R8R8},
	{"sm", 2, InstructionType::R8R8R8},
	{"b", 1, InstructionType::R8R8R8},
	{"bz", 2, InstructionType::R8R8R8},
	{"bnz", 2, InstructionType::R8R8R8},
	{"add", 3, InstructionType::R8R8R8},
	{"sub", 3, InstructionType::R8R8R8},
	{"not", 2, InstructionType::R8R8R8},
	{"and", 3, InstructionType::R8R8R8},
	{"or", 3, InstructionType::R8R8R8},
	{"xor", 3, InstructionType::R8R8R8},
	{"shl", 3, InstructionType::R8R8R8},
	{"shr", 3, InstructionType::R8R8R8},
	{"swb", 2, InstructionType::R8R8R8},
	{}
}};
