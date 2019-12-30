#include "memory.hpp"

#include <cstddef>
#include <fmt/core.h>
#include <stdexcept>

Mmu::Mmu(std::size_t bank_count) : ram(system_memory_size + bank_memory_size * bank_count) {}

Bank Mmu::set_current_bank(Bank new_bank)
{
	if (new_bank == Bank::Invalid || new_bank == Bank::Mmio || Byte(new_bank) < bank_count() + 1)
	{
		current_bank = new_bank;
	}
	else
	{
		fmt::print(stderr, "Note: Emulated machine set an invalid bank {}, setting as 0 (invalid)\n", Byte(new_bank));
		current_bank = Bank::Invalid;
	}

	return current_bank;
}

std::size_t Mmu::bank_count() const { return (ram.size() - system_memory_size) / bank_memory_size; }

std::size_t Mmu::ram_offset(Addr addr) const
{
	if (is_address_banked(addr))
	{
		switch (current_bank)
		{
		case Bank::Invalid:
		{
			throw std::runtime_error{fmt::format(
				"Attempted illegal byte access at address {:#06x}: tried "
				"addressing invalid bank",
				addr)};
		}

		case Bank::Mmio:
		{
			fmt::print(stderr, "Emulator bug, ram_offset() is invalid within MMIO areas");
			std::terminate();
		}

		default:
		{
			return addr + (std::size_t(current_bank) - std::size_t(Bank::UserBegin)) * bank_memory_size;
		}
		}
	}

	return addr;
}

bool Mmu::is_mmio(Addr addr) const { return is_address_banked(addr) && current_bank == Bank::Mmio; }

Byte Mmu::get_byte(Addr addr) const
{
	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		return mmio_read_callback(mmio_address(addr));
	}

	return ram[ram_offset(addr)];
}

void Mmu::set_byte(Addr addr, Byte data)
{
	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		mmio_write_callback(mmio_address(addr), data);
		return;
	}

	ram[ram_offset(addr)] = data;
}

Word Mmu::get_word(Addr addr) const { return (get_byte(addr + 0) << 0) | (get_byte(addr + 1) << 8); }
