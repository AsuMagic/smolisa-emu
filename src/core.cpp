#include <smol/core.hpp>

#include <smol/instruction.hpp>
#include <smol/opcodes.hpp>

#include <fmt/core.h>
#include <iostream>

void Core::dispatch()
{
	const Word instruction = mmu.get_word(registers[RegisterId::Ip]);
	current_instruction    = instruction;

	registers[RegisterId::Ip] += sizeof(Word);

	switch (Opcode(instruction & masks::opcode))
	{
	case Opcode::Li:
	{
		const auto [rdst, imm8] = formats::TypeI{instruction};
		registers[rdst] &= masks::upper_byte;
		registers[rdst] |= imm8;
		cycles += 2;
		break;
	}

	case Opcode::Liu:
	{
		const auto [rdst, imm8] = formats::TypeI{instruction};
		registers[rdst] &= masks::lower_byte;
		registers[rdst] |= imm8 << 8;
		cycles += 2;
		break;
	}

	case Opcode::Lb:
	{
		const auto [raddr, rdst, _r3] = formats::TypeR{instruction};
		registers[rdst]               = (registers[rdst] & masks::upper_byte) | mmu.get_byte(registers[raddr]);
		cycles += 4;
		break;
	}

	case Opcode::Sb:
	{
		const auto [raddr, rsrc, _r3] = formats::TypeR{instruction};
		mmu.set_byte(registers[raddr], registers[rsrc] & masks::lower_byte);
		cycles += 4;
		break;
	}

	case Opcode::Lw:
	{
		const auto [raddr, rdst, _r3] = formats::TypeR{instruction};
		registers[rdst]               = mmu.get_word(registers[raddr]);
		cycles += 4;
		break;
	}

	case Opcode::Sw:
	{
		const auto [raddr, rsrc, _r3] = formats::TypeR{instruction};
		mmu.set_word(registers[raddr], registers[rsrc]);
		cycles += 4;
		break;
	}

	case Opcode::Lrz:
	{
		const auto [rdst, rsrc, rcond] = formats::TypeR{instruction};

		if (registers[rcond] == 0)
		{
			registers[rdst] = registers[rsrc];
		}

		cycles += 3;
		break;
	}

	case Opcode::Lrnz:
	{
		const auto [rdst, rsrc, rcond] = formats::TypeR{instruction};

		if (registers[rcond] != 0)
		{
			registers[rdst] = registers[rsrc];
		}

		cycles += 3;
		break;
	}

	// TODO: lots of duplication below, but at the same time, we may want to apply specific side effects
	//       for each of those (e.g. flag updates, should we ever have that).
	case Opcode::Add:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] + registers[rb]; // TODO: side effects for arithmetic ops?
		cycles += 3;
		break;
	}

	case Opcode::Sub:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] - registers[rb];
		cycles += 3;
		break;
	}

	case Opcode::And:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] & registers[rb];
		cycles += 3;
		break;
	}

	case Opcode::Or:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] | registers[rb];
		cycles += 3;
		break;
	}

	case Opcode::Xor:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] ^ registers[rb];
		cycles += 3;
		break;
	}

	case Opcode::Shl:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] << registers[rb];
		cycles += 3;
		break;
	}

	case Opcode::Shr:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = registers[ra] >> registers[rb];
		cycles += 3;
		break;
	}

	case Opcode::Swb:
	{
		const auto [rdst, ra, rb] = formats::TypeR{instruction};
		registers[rdst]           = (registers[ra] & masks::upper_byte) >> 8 | (registers[rb] & masks::lower_byte) << 8;
		cycles += 3;

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

	// std::cout << debug_state() << '\n';

	++executed_ops;
}

void Core::boot()
{
	std::cout << debug_state_preamble() << '\n';
	std::cout << debug_state() << '\n';

	start_time = Timer::now();

	for (;;)
	{
		current_instruction.reset();
		dispatch();

		if (executed_ops % 100000 == 0)
		{
			const auto time_elapsed = std::chrono::duration<float>(Timer::now() - start_time).count();

			const float avg_cpi = float(cycles) / float(executed_ops);
			const float avg_mhz = (1.0e-6f * float(cycles)) / time_elapsed;

			fmt::print(
				"{:.3f}s: {:9} ins, {:9} cycles, avg CPI {:.3f}, avg MHz {:.3f} (R13={:04})\n",
				time_elapsed,
				executed_ops,
				cycles,
				avg_cpi,
				avg_mhz,
				registers[RegisterId(13)]);
		}
	}
}

auto Core::debug_state_multiline() const -> std::string
{
	std::string ret = "\nRegister dump:\n";

	for (std::size_t i = 0; i < RegisterFile::register_count; ++i)
	{
		ret += fmt::format("${:<4}: {:#06x}\n", register_name(RegisterId(i)), registers[RegisterId(i)]);
	}

	if (current_instruction)
	{
		ret += fmt::format("opcode: {:#06x}\n", *current_instruction);
	}
	else
	{
		ret += fmt::format("opcode: failed\n");
	}

	return ret;
}

auto Core::debug_state() const -> std::string
{
	std::string ret;

	for (std::size_t i = 0; i < RegisterFile::register_count; ++i)
	{
		ret += fmt::format("{:04x} ", registers[RegisterId(i)]);
	}

	if (current_instruction)
	{
		ret += fmt::format("{:04x}", *current_instruction);
	}
	else
	{
		ret += "0000";
	}

	return ret;
}

std::string Core::debug_state_preamble() const
{
	std::string ret;

	for (std::size_t i = 0; i < RegisterFile::register_count; ++i)
	{
		ret += fmt::format("{:<5}", register_name(RegisterId(i)));
	}

	ret += "(op) ";

	return ret;
}
