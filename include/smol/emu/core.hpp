#pragma once

#include "smol/common/registers.hpp"
#include "smol/emu/memory.hpp"

#include <array>
#include <optional>
#include <string>

struct RegisterFile
{
	static constexpr std::size_t register_count = 16;

	std::array<Word, register_count> data = {};

	auto operator[](RegisterId id) -> Word& { return data.at(std::size_t(id)); }
	auto operator[](RegisterId id) const -> const Word& { return data.at(std::size_t(id)); }
};

enum class DebugTraceStyle
{
	Multiline,
	Oneline
};

struct Core
{
	RegisterFile        registers;
	std::optional<Word> current_instruction;
	Mmu                 mmu;
	// std::size_t         retired_instructions = 0;

	std::function<void(Core&)> panic_handler;

	void dispatch();
	void boot();

	[[nodiscard]] auto debug_state(DebugTraceStyle style) const -> std::string;
	void               trace(std::ostream& out);
};
