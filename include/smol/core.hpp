#pragma once

#include <smol/memory.hpp>
#include <smol/registers.hpp>

#include <array>
#include <chrono>
#include <optional>
#include <string>

struct RegisterFile
{
	static constexpr std::size_t register_count = 16;

	std::array<Word, register_count> data = {};

	auto operator[](RegisterId id) -> Word& { return data.at(std::size_t(id)); }
	auto operator[](RegisterId id) const -> const Word& { return data.at(std::size_t(id)); }
};

struct Core
{
	RegisterFile       registers;
	std::optional<u32> current_instruction;
	Mmu                mmu;

	std::size_t executed_ops = 0;
	std::size_t cycles       = 0;

	using Timer = std::chrono::high_resolution_clock;
	Timer::time_point start_time;

	std::function<void(Core&)> panic_handler;

	void dispatch();
	void boot();

	[[nodiscard]] auto debug_state_multiline() const -> std::string;
	[[nodiscard]] auto debug_state() const -> std::string;
	[[nodiscard]] auto debug_state_preamble() const -> std::string;
};
