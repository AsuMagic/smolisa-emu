#pragma once

#include "smol/common/types.hpp"

#include <functional>
#include <vector>

enum class Bank : Word
{
	Invalid   = 0x0000,
	UserBegin = 0x0001,
	UserEnd   = 0xFFFE,
	Mmio      = 0xFFFF
};

struct Mmu
{
	static constexpr auto address_space_size = (1 << (sizeof(Word) * 8));
	static constexpr auto system_memory_size = 8192;
	static constexpr auto bank_memory_size   = address_space_size - system_memory_size;

	static constexpr auto is_address_banked(Addr addr) -> bool { return addr >= system_memory_size; }

	static constexpr auto mmio_address(Addr real_address) -> Addr { return real_address - system_memory_size; }

	std::vector<Byte> ram;

	Bank current_bank = Bank::Invalid;

	std::function<Byte(Addr)>       mmio_read_callback;
	std::function<void(Addr, Byte)> mmio_write_callback;

	explicit Mmu(std::size_t bank_count = 128);

	auto set_current_bank(Bank new_bank) -> Bank;

	[[nodiscard]] auto bank_count() const -> std::size_t;
	[[nodiscard]] auto ram_offset(Addr addr) const -> std::size_t;

	[[nodiscard]] auto is_mmio(Addr addr) const -> bool;

	[[nodiscard]] auto get_byte(Addr addr) const -> Byte;
	void               set_byte(Addr addr, Byte data);

	[[nodiscard]] auto get_word(Addr addr) const -> Word;
	void               set_word(Addr addr, Word data);
};
