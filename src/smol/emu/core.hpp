#pragma once

#include <smol/common/registers.hpp>
#include <smol/emu/memory.hpp>

#include <array>
#include <optional>

struct RegisterFile
{
	static constexpr std::size_t register_count = 16;

	std::array<Word, register_count> data = {};

	Word&       operator[](RegisterId id) { return data[std::size_t(id)]; }
	const Word& operator[](RegisterId id) const { return data[std::size_t(id)]; }
};

struct Core
{
	RegisterFile        registers;
	Word                instruction_pointer = 0x0000;
	std::optional<Word> current_instruction;
	Mmu                 mmu;

	std::function<void(Core&)> panic_handler;

	void dispatch();
	void boot();

	std::string debug_state() const;
};
