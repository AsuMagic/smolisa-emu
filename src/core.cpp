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

		if (fetch_state != AccessStatus::Ok) [[unlikely]]
		{
			fire_exception("Opcode fetch failed (u16 #1)");
			return;
		}

		current_instruction = first_word;
	}

	{
		auto [fetch_state, second_word] = mmu.get_u16(rip + 2);

		if (fetch_state != AccessStatus::Ok) [[unlikely]]
		{
			fire_exception("Opcode fetch failed (u16 #2)");
			return;
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
		[&](CLR x) {
			if (t_bit)
			{
				regs[x.dst] = regs[x.src];
			}
			rip += 2;
		},
		// TODO: l8ow l16ow l32ow
		[&](LR x) {
			regs[x.dst] = regs[x.src];
			rip += 2;
		},
		[&](LS8 x) {
			const auto [state, value] = mmu.get_u8(regs[x.addr]);

			if (check_access_or_fault(state))
			{
				regs[x.dst] = s32(s8(value));
				rip += 2;
			}
		},
		[&](LS16 x) {
			const auto [state, value] = mmu.get_u16(regs[x.addr]);

			if (check_access_or_fault(state))
			{
				regs[x.dst] = s32(s16(value));
				rip += 2;
			}
		},
		// TODO: ls8ow, ls16ow, l8o, l16o, l32o, ls8o, ls16o
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
			fmt::print("BRK called @{}\n", rip);
			rip += 2;
		},
		[&](S8 x) {
			const auto state = mmu.set_u8(regs[x.addr], regs[x.src]);

			if (check_access_or_fault(state))
			{
				rip += 2;
			}
		},
		[&](S8O x) {
			const auto state = mmu.set_u8(regs[x.base_addr] + x.offset, regs[x.src]);

			if (check_access_or_fault(state))
			{
				rip += 2;
			}
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
			const s32 computed_address = regs[RegisterId::RPL] + x.offset * 4;
			const auto [state, value] = mmu.get_u32(computed_address);

			if (check_access_or_fault(state))
			{
				regs[x.dst] = value;
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
		[&](BAND x) {
			regs[x.a_dst] &= regs[x.b];
			rip += 2;
		},
		[&](BSLI x) {
			regs[x.a_dst] <<= x.b;
			rip += 2;
		},
		[&](BSRITLSB x) {
			regs[x.a_dst] >>= x.b;
			t_bit = (regs[x.a_dst] & 0b1) != 0;
			rip += 2;
		},
		[&](BASRI x) {
			regs[x.a_dst] = s32(regs[x.a_dst]) >> x.b;
			rip += 2;
		},
		[&](INTOFF x) {
			interrupts.enabled = false;
			rip += 2;
		},
		[&](INTON x) {
			interrupts.enabled = true;
			rip += 2;
		},
		[&](INTRET x) {
			interrupts.enabled = true;
			rip = interrupts.intret;
			rip += 2;
		},
		[&](INTWAIT x) {
			if (!interrupts.enabled)
			{
				throw std::runtime_error{
					"Core waiting for interrupt but interrupts are disabled"
				};
			}

			throw std::runtime_error{"Waiting for interrupts unimplemented"};
			rip += 2;
		},
		[&](Unknown) { fire_exception("Illegal instruction"); },
		[&](auto op) { fire_exception("Unimplemented instruction"); },
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

auto Core::fire_interrupt(Word id) -> bool
{
	if (!interrupts.enabled)
	{
		return false;
	}

	interrupts.enabled = false;
	interrupts.intret = rip;

	// TODO: magic constants begone
	const Word address = (0x00001000 + id * 16);

	rip = address;

	return true;
}

void Core::fire_exception(std::string_view reason)
{
	if (!fire_interrupt(0)) [[unlikely]]
	{
		throw std::runtime_error{
			fmt::format(
				"{} (interrupts disabled)",
				reason
			)
		};
	}
}

auto Core::check_access_or_fault(AccessStatus status) -> bool
{
	if (status == AccessStatus::Ok)
	{
		return true;
	}

	fire_exception(access_status_strings.at(int(status)));
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
