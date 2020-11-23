#include "smol/emu/memory.hpp"

#include <cstddef>
#include <fmt/core.h>
#include <stdexcept>

Mmu::Mmu(std::size_t bank_count) : ram(system_memory_size + bank_memory_size * bank_count) {}

auto Mmu::set_current_bank(Bank new_bank) -> Bank
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

auto Mmu::bank_count() const -> std::size_t { return (ram.size() - system_memory_size) / bank_memory_size; }

auto Mmu::ram_offset(Addr addr) const -> std::size_t
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
			// fmt::print(stderr, "bk read @{:#06x}\n", addr);
			return addr + (std::size_t(current_bank) - std::size_t(Bank::UserBegin)) * bank_memory_size;
		}
		}
	}

	return addr;
}

auto Mmu::is_mmio(Addr addr) const -> bool { return is_address_banked(addr) && current_bank == Bank::Mmio; }

auto Mmu::get_byte(Addr addr) const -> Byte
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

auto Mmu::get_word(Addr addr) const -> Word { return (get_byte(addr + 0) << 0) | (get_byte(addr + 1) << 8); }

void Mmu::set_word(Addr addr, Word data)
{
	set_byte(addr, Byte(data & 0xFF));
	set_byte(addr + 1, Byte((data >> 8) & 0xFF));
}
