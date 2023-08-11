#include "smol/memory.hpp"
#include "smol/registers.hpp"
#include "smol/util.hpp"
#include <smol/core.hpp>

#include <smol/instruction.hpp>

#include <fmt/core.h>
#include <iostream>
#include <stdexcept>

void Core::dispatch()
{
	using namespace insns;

	{
		auto [fetch_state, first_word] = mmu.get_u16(rip);

		if (fetch_state != AccessStatus::Ok)
		{
			// TODO: more details
			throw std::runtime_error{"Opcode fetch failed"};
		}

		current_instruction = first_word;
	}

	{
		auto [fetch_state, second_word] = mmu.get_u16(rip + 2);

		if (fetch_state != AccessStatus::Ok)
		{
			// TODO: more details
			throw std::runtime_error{"Opcode fetch of 2nd u16 fetch failed"};
		}

		*current_instruction |= second_word << 16;
	}

	const auto decoded_ins = insns::decode(*current_instruction);
	// fmt::print("{}| {}\n", debug_state(), disassemble(decoded_ins));

	std::visit(overloaded{
		[&](L8 x) {
			const auto [state, value] = mmu.get_u8(regs[x.addr]);

			if (check_access_or_fault(state))
			{
				regs[x.dst] = value;
				rip += 2;
			}
		},
		[&](L16 x) {
			const auto [state, value] = mmu.get_u16(regs[x.addr]);

			if (check_access_or_fault(state))
			{
				regs[x.dst] = value;
				rip += 2;
			}
		},
		[&](L32 x) {
			const auto [state, value] = mmu.get_u32(regs[x.addr]);

			if (check_access_or_fault(state))
			{
				regs[x.dst] = value;
				rip += 2;
			}
		},
		[&](LSI x) {
			regs[x.dst] = x.imm;
			rip += 2;
		},
		[&](LSIW x) {
			regs[x.dst] = x.imm;
			rip += 4;
		},
		[&](LIPREL x) {
			regs[x.dst] = rip + 2 + (x.imm << 1);
			rip += 4;
		},
		[&](BRK x) {
			fmt::print("Hit BRK");
			rip += 2;
		},
		[&](TLTU x) {
			t_bit = regs[x.a] < regs[x.b];
			rip += 2;
		},
		[&](TLTS x) {
			t_bit = s32(regs[x.a]) < s32(regs[x.b]);
			rip += 2;
		},
		[&](TGEU x) {
			t_bit = regs[x.a] >= regs[x.b];
			rip += 2;
		},
		[&](TGES x) {
			t_bit = s32(regs[x.a]) >= s32(regs[x.b]);
			rip += 2;
		},
		[&](TE x) {
			t_bit = regs[x.a] == regs[x.b];
			rip += 2;
		},
		[&](TNE x) {
			t_bit = regs[x.a] != regs[x.b];
			rip += 2;
		},
		// TODO: tltsi, tgesi, tbz
		[&](TEI x) {
			t_bit = regs[x.a] == x.b;
			rip += 2;
		},
		[&](TNEI x) {
			t_bit = regs[x.a] != x.b;
			rip += 2;
		},
		[&](PLL32 x) {
			const s32 computed_address = regs[RegisterId::RPL] + x.index * 4;
			const auto [state, value] = mmu.get_u32(computed_address);

			if (check_access_or_fault(state))
			{
				regs[x.dst] = value;
				rip += 2;
			}
		},
		[&](S8 x) {
			const auto state = mmu.set_u8(regs[x.addr], regs[x.src]);

			if (check_access_or_fault(state))
			{
				rip += 2;
			}
		},
		[&](J x) {
			rip = regs[x.target];
		},
		[&](CJ x) {
			if (t_bit)
			{
				rip = regs[x.target];
			}
			else
			{
				rip += 2;
			}
		},
		[&](JAL x) {
			regs[x.dst] = rip + 2;
			rip = regs[x.target];
		},
		[&](JALI x) {
			regs[RegisterId::RRET] = rip + 2;
			rip = rip + 2 + (x.relative_target << 1);
		},
		[&](CJI x) {
			if (t_bit)
			{
				rip = rip + 2 + (x.relative_target << 1);
			}
			else
			{
				rip += 2;
			}
		},
		[&](IADD x) {
			regs[x.a_dst] += regs[x.b];
			rip += 2;
		},
		[&](IADDSI x) {
			regs[x.a_dst] = s32(regs[x.a_dst]) + x.b;
			rip += 2;
		},
		[&](IADDSIW x) {
			regs[x.dst] = s32(regs[x.a]) + x.b;
			rip += 4;
		},
		[&](IADDSITNZ x) {
			const auto sum = s32(regs[x.a_dst]) + x.b;
			regs[x.a_dst] = sum;
			t_bit = (sum != 0);
			rip += 2;
		},
		[&](BSLI x) {
			regs[x.a_dst] <<= x.b;
			rip += 2;
		},
		[&](BSRI x) {
			regs[x.a_dst] >>= x.b;
			rip += 2;
		},
		[&](BASRI x) {
			regs[x.a_dst] = s32(regs[x.a_dst]) >> x.b;
			rip += 2;
		},
		[&](Unknown) { throw std::runtime_error("Illegal opcode"); },
		[&](auto op) { throw std::runtime_error("Unimplemented opcode"); },
	}, decoded_ins);

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

		if (executed_ops % 10000000 == 0)
		{
			const auto time_elapsed = std::chrono::duration<float>(Timer::now() - start_time).count();

			const float avg_mhz = (1.0e-6f * float(executed_ops)) / time_elapsed;

			fmt::print(
				"{:.3f}s: {:9} ins, avg MHz {:.3f}\n",
				time_elapsed,
				executed_ops,
				avg_mhz
			);
		}

		if (executed_ops % 10000 == 0)
		{
			if (keepalive)
			{
				keepalive();
			}
		}
	}
}

auto Core::check_access_or_fault(AccessStatus status) -> bool
{
	if (status == AccessStatus::Ok)
	{
		return true;
	}

	// TODO: proper interrupt handling
	throw std::runtime_error{
		std::string(access_status_strings.at(int(status)))
	};
}

auto Core::debug_state_multiline() const -> std::string
{
	std::string ret = "\nRegister dump:\n";

	for (std::size_t i = 0; i < RegisterFile::register_count; ++i)
	{
		ret += fmt::format("{:<4}: {:#010x}\n", register_name(RegisterId(i)), regs[RegisterId(i)]);
	}

	ret += fmt::format("rip : {:#010x}\n", rip);

	if (current_instruction)
	{
		ret += fmt::format("*rip: {:#010x}\n", *current_instruction);
	}
	else
	{
		ret += fmt::format("*rip: failed\n");
	}

	return ret;
}

auto Core::debug_state() const -> std::string
{
	std::string ret;

	for (std::size_t i = 0; i < RegisterFile::register_count; ++i)
	{
		ret += fmt::format("{:08x} ", regs[RegisterId(i)]);
	}

	if (current_instruction)
	{
		ret += fmt::format("{:08x} ", *current_instruction);
	}
	else
	{
		ret += "???????? ";
	}

	ret += fmt::format("{:08x} ", rip);

	return ret;
}

std::string Core::debug_state_preamble() const
{
	std::string ret;

	for (std::size_t i = 0; i < RegisterFile::register_count; ++i)
	{
		ret += fmt::format("{:<9}", register_name(RegisterId(i)));
	}

	ret += "(op)     ";
	ret += "(rip)    ";

	return ret;
}
