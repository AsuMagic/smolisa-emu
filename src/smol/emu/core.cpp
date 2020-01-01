#include "smol/emu/core.hpp"

#include "smol/common/instruction.hpp"
#include "smol/common/opcodes.hpp"

#include <fmt/core.h>
#include <iostream>

void Core::dispatch()
{
	if ((instruction_pointer & 0b1) != 0)
	{
		throw std::runtime_error{"Invalid alignment: Instruction pointer should be aligned to 2 bytes"};
	}

	const Word instruction = mmu.get_word(instruction_pointer);
	current_instruction    = instruction;

	Word next_instruction_pointer = instruction_pointer + sizeof(Word);

	if (instruction == 0xFFFF)
	{
		throw std::runtime_error{"Magic quit instruction value encountered."};
	}

	switch (Opcode(instruction & masks::opcode))
	{
	case Opcode::Li:
	{
		const auto [rdst, imm8] = formats::TypeI{instruction};
		registers[rdst] &= masks::upper_byte;
		registers[rdst] |= imm8;
		break;
	}

	case Opcode::Lm:
	{
		const auto [raddr, rdst, _r3] = formats::TypeR{instruction};
		registers[rdst] &= masks::upper_byte;
		registers[rdst] |= mmu.get_byte(registers[raddr]);
		break;
	}

	case Opcode::Sm:
	{
		const auto [raddr, rsrc, _r3] = formats::TypeR{instruction};
		mmu.set_byte(registers[raddr], registers[rsrc] & masks::lower_byte);
		break;
	}

	case Opcode::B:
	{
		const auto [raddr, _r2, _r3] = formats::TypeR{instruction};
		next_instruction_pointer     = registers[raddr];
		break;
	}

	case Opcode::Bz:
	{
		const auto [raddr, rtest, _r3] = formats::TypeR{instruction};

		if (registers[rtest] == 0)
		{
			next_instruction_pointer = registers[raddr];
		}

		break;
	}

	case Opcode::Bnz:
	{
		const auto [raddr, rtest, _r3] = formats::TypeR{instruction};

		if (registers[rtest] != 0)
		{
			next_instruction_pointer = registers[raddr];
		}

		break;
	}

	// TODO: lots of duplication below, but at the same time, we may want to apply specific side effects
	//       for each of those (e.g. flag updates, should we ever have that).
	case Opcode::Add:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] + registers[rb]; // TODO: side effects for arithmetic ops?
		break;
	}

	case Opcode::Sub:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] - registers[rb];
		break;
	}

	case Opcode::Not:
	{
		const auto [rdst, ra, _r3] = formats::TypeR{instruction};
		registers[rdst]            = ~registers[ra];
		break;
	}

	case Opcode::And:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] & registers[rb];
		break;
	}

	case Opcode::Or:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] | registers[rb];
		break;
	}

	case Opcode::Xor:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] ^ registers[rb];
		break;
	}

	case Opcode::Shl:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] << registers[rb];
		break;
	}

	case Opcode::Shr:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] >> registers[rb];
		break;
	}

	case Opcode::Swb:
	{
		const auto [rdst, ra, _r3] = formats::TypeR{instruction};
		registers[rdst] = (registers[ra] & masks::upper_byte) >> 8 | (registers[ra] & masks::lower_byte) << 8;

		break;
	}

	default:
	{
		throw std::runtime_error{"Invalid opcode!"};
	}
	}

	// HACK: We should probably not update the MMU bank register every time.
	//       Only check for changes when actually accessing memory?
	registers[RegisterId::Bank] = Word(mmu.set_current_bank(Bank(registers[RegisterId::Bank])));

	instruction_pointer = next_instruction_pointer;
}

void Core::boot()
{
	for (;;)
	{
		current_instruction.reset();
		dispatch();
	}
}

auto Core::debug_state() const -> std::string
{
	std::string ret;

	ret += "\nRegister dump:\n";
	for (std::size_t i = 0; i < RegisterFile::register_count; ++i)
	{
		// IIFE
		const auto name = [i]() -> std::string {
			switch (i)
			{
			case 15: return "bank";
			default: return fmt::format("g{}", i);
			}
		}();

		ret += fmt::format("${:<4}: {:#06x}\n", name, registers[RegisterId(i)]);
	}

	ret += fmt::format("Instruction pointer: {:#06x}\n", instruction_pointer);

	if (current_instruction)
	{
		ret += fmt::format("Current instruction: {:#06x}\n", *current_instruction);
		// TODO: disasm
	}
	else
	{
		ret += fmt::format("No instruction could be read");
	}

	return ret;
}
