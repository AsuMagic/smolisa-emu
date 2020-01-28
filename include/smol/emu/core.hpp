#pragma once

#include "smol/common/registers.hpp"
#include "smol/emu/memory.hpp"

#include <array>
#include <optional>

struct RegisterFile
{
	static constexpr std::size_t register_count = 16;

	std::array<Word, register_count> data = {};

	auto operator[](RegisterId id) -> Word& { return data.at(std::size_t(id)); }
	auto operator[](RegisterId id) const -> const Word& { return data.at(std::size_t(id)); }
};

struct Core
{
	RegisterFile        registers;
	Word                instruction_pointer = 0x0000;
	std::optional<Word> current_instruction;
	Mmu                 mmu;
	// std::size_t         retired_instructions = 0;

	std::function<void(Core&)> panic_handler;

	void dispatch();
	void boot();

	[[nodiscard]] auto debug_state() const -> std::string;
};
