#include "smol/memory.hpp"
#include "smol/registers.hpp"
#include "smol/util.hpp"
#include <smol/core.hpp>

#include <smol/instruction.hpp>

#include <fmt/core.h>
#include <iostream>
#include <stdexcept>
#include <type_traits>

auto Core::fetch_instruction_u32() -> std::optional<u32>
{
	u16 lower_word = 0;
	u16 upper_word = 0;

	// we perform two separate fetches here because we allow an alignment of %2.
	//
	// ideally we would only check on the second word when required, but taking
	// this shortcut simplifies things and doesn't cause issues unless we run
	// into some odd edge cases (e.g. executing at the last word of a page).
	// so we might need to change this behavior later on

	{
		AccessStatus fetch_state{};
		std::tie(fetch_state, lower_word) = mmu.get_u16(rip);

		if (fetch_state != AccessStatus::Ok) [[unlikely]]
		{
			fire_exception("Opcode fetch failed (u16 #1)");
			return std::nullopt;
		}
	}

	{
		AccessStatus fetch_state{};
		std::tie(fetch_state, upper_word) = mmu.get_u16(rip + 2);

		if (fetch_state != AccessStatus::Ok) [[unlikely]]
		{
			fire_exception("Opcode fetch failed (u16 #2)");
			return std::nullopt;
		}
	}

	return lower_word | (upper_word << 16U);
}

void Core::execute_single()
{
	using namespace insns;

	current_instruction = fetch_instruction_u32();

	if (!current_instruction.has_value())
	{
		// fault has occurred; next execute_single will hit the fault handler
		return;
	}

	const auto decoded_ins = insns::decode(*current_instruction);

	if (verbose_exec)
	{
		fmt::print("{}| {}\n", debug_state(), disassemble(decoded_ins));
	}

	const auto instruction_width = std::visit([&](auto x) { return x.length; }, decoded_ins);
	next_rip                     = rip + instruction_width;

	const auto check_load = [this](auto fetched, Word& dst, bool sext = false) {
		auto [state, value] = fetched;

		if (!check_access_else_fault(state))
		{
			return;
		}

		if (sext)
		{
			value = std::make_signed_t<decltype(value)>(value);
			dst   = s32(value);
		}
		else
		{
			dst = value;
		}
	};

	const auto check_store = [this](auto state) {
		check_access_else_fault(state);
	};

	static const auto handle_op = overloaded{
		[&](L8 x) { check_load(mmu.get_u8(regs[x.addr]), regs[x.dst]); },
		[&](L16 x) { check_load(mmu.get_u16(regs[x.addr]), regs[x.dst]); },
		[&](L32 x) { check_load(mmu.get_u32(regs[x.addr]), regs[x.dst]); },

		[&](CLR x) {
			if (t_bit)
			{
				regs[x.dst] = regs[x.src];
			}
		},

		[&](L8OW x) { check_load(mmu.get_u8(regs[x.base_addr] + x.offset), regs[x.dst]); },
		[&](L16OW x) { check_load(mmu.get_u16(regs[x.base_addr] + (x.offset << 1)), regs[x.dst]); },
		[&](L32OW x) { check_load(mmu.get_u32(regs[x.base_addr] + (x.offset << 2)), regs[x.dst]); },

		[&](LR x) { regs[x.dst] = regs[x.src]; },

		[&](LS8 x) { check_load(mmu.get_u8(regs[x.addr]), regs[x.dst], true); },
		[&](LS16 x) { check_load(mmu.get_u16(regs[x.addr]), regs[x.dst], true); },

		[&](LS8OW x) { check_load(mmu.get_u8(regs[x.base_addr] + x.offset), regs[x.dst], true); },
		[&](LS16OW x) { check_load(mmu.get_u16(regs[x.base_addr] + (x.offset << 1)), regs[x.dst], true); },

		[&](L8O x) { check_load(mmu.get_u8(regs[x.base_addr] + x.offset), regs[x.dst]); },
		[&](L16O x) { check_load(mmu.get_u16(regs[x.base_addr] + (x.offset << 1)), regs[x.dst]); },
		[&](L32O x) { check_load(mmu.get_u32(regs[x.base_addr] + (x.offset << 2)), regs[x.dst]); },

		[&](LS8O x) { check_load(mmu.get_u8(regs[x.base_addr] + x.offset), regs[x.dst], true); },
		[&](LS16O x) { check_load(mmu.get_u16(regs[x.base_addr] + (x.offset << 1)), regs[x.dst], true); },

		[&](LSI x) { regs[x.dst] = x.imm; },
		[&](LSIH x) { regs[x.dst] = (regs[x.dst] & 0x00FF'FFFFU) | (x.imm << 24); },
		[&](LSIW x) { regs[x.dst] = x.imm; },

		[&](LIPREL x) { regs[x.dst] = rip + 2 + (x.imm << 1); },

		[&](S8 x) { check_store(mmu.set_u8(regs[x.addr], regs[x.src])); },
		[&](S16 x) { check_store(mmu.set_u16(regs[x.addr], regs[x.src])); },
		[&](S32 x) { check_store(mmu.set_u32(regs[x.addr], regs[x.src])); },

		[&](PUSH x) {
			regs[RegisterId::RPS] -= 4;
			check_store(mmu.set_u32(regs[RegisterId::RPS], regs[x.src]));
		},

		[&](S8OW x) { check_store(mmu.set_u8(regs[x.base_addr] + x.offset, regs[x.src])); },
		[&](S16OW x) { check_store(mmu.set_u16(regs[x.base_addr] + (x.offset << 1), regs[x.src])); },
		[&](S32OW x) { check_store(mmu.set_u32(regs[x.base_addr] + (x.offset << 2), regs[x.src])); },

		[&](S8O x) { check_store(mmu.set_u8(regs[x.base_addr] + x.offset, regs[x.src])); },
		[&](S16O x) { check_store(mmu.set_u16(regs[x.base_addr] + (x.offset << 1), regs[x.src])); },
		[&](S32O x) { check_store(mmu.set_u32(regs[x.base_addr] + (x.offset << 2), regs[x.src])); },

		[&](BRK x) { fmt::print("BRK called @{}\n", rip); },

		[&](TLTU x) { t_bit = regs[x.a] < regs[x.b]; },
		[&](TLTS x) { t_bit = s32(regs[x.a]) < s32(regs[x.b]); },
		[&](TGEU x) { t_bit = regs[x.a] >= regs[x.b]; },
		[&](TGES x) { t_bit = s32(regs[x.a]) >= s32(regs[x.b]); },
		[&](TE x) { t_bit = regs[x.a] == regs[x.b]; },
		[&](TNE x) { t_bit = regs[x.a] != regs[x.b]; },
		[&](TLTSI x) { t_bit = s32(regs[x.a]) < s32(x.b); },
		[&](TGESI x) { t_bit = s32(regs[x.a]) >= s32(x.b); },
		[&](TBZ x) {
			const auto v = regs[x.a]; 
			t_bit = (
				((v & 0x00'00'00'FF) == 0) ||
				((v & 0x00'00'FF'00) == 0) ||
				((v & 0x00'FF'00'00) == 0) ||
				((v & 0xFF'00'00'00) == 0)
			);
		},
		[&](TEI x) { t_bit = regs[x.a] == x.b; },
		[&](TNEI x) { t_bit = regs[x.a] != x.b; },

		[&](PLL32 x) { check_load(mmu.get_u32(regs[RegisterId::RPL] + (x.offset << 2)), regs[x.dst]); },

		[&](J x) { next_rip = regs[x.target]; },
		[&](CJ x) {
			if (t_bit)
			{
				next_rip = regs[x.target];
			}
		},
		[&](JAL x) {
			regs[x.dst] = rip + 2;
			next_rip    = regs[x.target];
		},
		[&](JALI x) {
			regs[RegisterId::RRET] = rip + 2;
			next_rip               = rip + 2 + (x.relative_target << 1);
		},
		[&](CJI x) {
			if (t_bit)
			{
				next_rip = rip + 2 + (x.relative_target << 1);
			}
		},

		[&](BSEXT8 x) { regs[x.a_dst] = s32(s8(regs[x.b])); },
		[&](BSEXT16 x) { regs[x.a_dst] = s32(s16(regs[x.b])); },
		[&](BZEXT8 x) { regs[x.a_dst] = u8(regs[x.b]); },
		[&](BZEXT16 x) { regs[x.a_dst] = u16(regs[x.b]); },

		[&](INEG x) { regs[x.a_dst] = -regs[x.b]; },
		[&](ISUB x) { regs[x.a_dst] -= regs[x.b]; },
		[&](IADD x) { regs[x.a_dst] += regs[x.b]; },
		[&](IADDSI x) { regs[x.a_dst] = s32(regs[x.a_dst]) + x.b; },
		[&](IADDSIW x) { regs[x.dst] = s32(regs[x.a]) + x.b; },
		[&](IADDSITNZ x) {
			const auto sum = s32(regs[x.a_dst]) + x.b;
			regs[x.a_dst]  = sum;
			t_bit          = (sum != 0);
		},

		[&](BAND x) { regs[x.a_dst] &= regs[x.b]; },
		[&](BOR x) { regs[x.a_dst] |= regs[x.b]; },
		[&](BXOR x) { regs[x.a_dst] ^= regs[x.b]; },
		[&](BSL x) { regs[x.a_dst] <<= regs[x.b]; },
		[&](BSR x) { regs[x.a_dst] >>= regs[x.b]; },
		[&](BASR x) { regs[x.a_dst] = s32(regs[x.a_dst]) >> regs[x.b]; },
		[&](BSLI x) { regs[x.a_dst] <<= x.b; },
		[&](BSRITLSB x) {
			regs[x.a_dst] >>= x.b;
			t_bit = (regs[x.a_dst] & 0b1) != 0;
		},
		[&](BASRI x) { regs[x.a_dst] = s32(regs[x.a_dst]) >> x.b; },

		[&](INTOFF x) { interrupts.enabled = false; },
		[&](INTON x) { interrupts.enabled = true; },
		[&](INTRET x) {
			interrupts.enabled = true;
			rip                = interrupts.intret;
		},
		[&](INTWAIT x) {
			if (!interrupts.enabled)
			{
				throw std::runtime_error{"Core waiting for interrupt but interrupts are disabled"};
			}

			throw std::runtime_error{"Waiting for interrupts unimplemented"};
		},

		[&](Unknown) { fire_exception("Illegal instruction"); },

		// [&](auto) { fire_exception("Unimplemented instruction"); },
	};

	std::visit(handle_op, decoded_ins);
	rip = next_rip;

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
		execute_single();

		if (executed_ops % 10000000 == 0)
		{
			const auto time_elapsed = std::chrono::duration<float>(Timer::now() - start_time).count();

			const float avg_mhz = (1.0e-6F * float(executed_ops)) / time_elapsed;

			fmt::print("{:.3f}s: {:9} ins, avg MHz {:.3f}\n", time_elapsed, executed_ops, avg_mhz);
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
	interrupts.intret  = rip;

	// TODO: magic constants begone
	const Word address = (0x00001000 + id * 16);

	rip = address;

	return true;
}

void Core::fire_exception(std::string_view reason)
{
	if (!fire_interrupt(0)) [[unlikely]]
	{
		throw std::runtime_error{fmt::format("{} (interrupts disabled)", reason)};
	}

	next_rip = rip;
}

auto Core::check_access_else_fault(AccessStatus status) -> bool
{
	if (status == AccessStatus::Ok)
	{
		return true;
	}

	fire_exception(access_status_strings.at(int(status)));
	return false;
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

auto Core::debug_state_preamble() const -> std::string
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
