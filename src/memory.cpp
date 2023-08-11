#include <smol/memory.hpp>

#include <cstddef>
#include <fmt/core.h>
#include <stdexcept>

Mmu::Mmu() : ram(system_memory_size) {}

auto Mmu::get_u8(Addr addr) const -> std::pair<AccessStatus, u8>
{
	if (!is_mapped(addr))
	{
		return {AccessStatus::ErrorUnmapped, 0};
	}

	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		const auto [err, v] = mmio_read_callback(mmio_address(addr), AccessGranularity::U8);
		return {err, u8(v)};
	}

	return {AccessStatus::Ok, ram[addr]};
}

auto Mmu::set_u8(Addr addr, u8 data) -> AccessStatus
{
	if (!is_mapped(addr))
	{
		return AccessStatus::ErrorUnmapped;
	}

	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		return mmio_write_callback(mmio_address(addr), data, AccessGranularity::U8);
	}

	ram[addr] = data;
	return AccessStatus::Ok;
}

auto Mmu::get_u16(Addr addr) const -> std::pair<AccessStatus, u16>
{
	if (!is_mapped(addr))
	{
		return {AccessStatus::ErrorUnmapped, 0};
	}

	if ((addr & 0b1) != 0)
	{
		return {AccessStatus::ErrorMisaligned, 0};
	}

	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		const auto [err, v] = mmio_read_callback(mmio_address(addr), AccessGranularity::U16);
		return {err, u16(v)};
	}

	return {
		AccessStatus::Ok,
		ram[addr] | (ram[addr + 1] << 8)
	};
}

auto Mmu::set_u16(Addr addr, u16 data) -> AccessStatus
{
	if (!is_mapped(addr))
	{
		return AccessStatus::ErrorUnmapped;
	}

	if ((addr & 0b1) != 0)
	{
		return AccessStatus::ErrorMisaligned;
	}

	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		return mmio_write_callback(mmio_address(addr), data, AccessGranularity::U16);
	}

	ram[addr] = data & 0xFF;
	ram[addr + 1] = data >> 8;
	return AccessStatus::Ok;
}


auto Mmu::get_u32(Addr addr) const -> std::pair<AccessStatus, u32>
{
	if (!is_mapped(addr))
	{
		return {AccessStatus::ErrorUnmapped, 0};
	}

	if ((addr & 0b11) != 0)
	{
		return {AccessStatus::ErrorMisaligned, 0};
	}

	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		const auto [err, v] = mmio_read_callback(mmio_address(addr), AccessGranularity::U32);
		return {err, v};
	}

	return {
		AccessStatus::Ok,
		ram[addr] | (ram[addr + 1] << 8) | (ram[addr + 2] << 12) | (ram[addr + 3] << 16)
	};
}

auto Mmu::set_u32(Addr addr, u32 data) -> AccessStatus
{
	if (!is_mapped(addr))
	{
		return AccessStatus::ErrorUnmapped;
	}

	if ((addr & 0b11) != 0)
	{
		return AccessStatus::ErrorMisaligned;
	}

	if (is_mmio(addr))
	{
		// Fails if MMIO is not set up
		return mmio_write_callback(mmio_address(addr), data, AccessGranularity::U32);
	}

	ram[addr + 0] = (data >> 0) & 0xFF;
	ram[addr + 1] = (data >> 8) & 0xFF;
	ram[addr + 2] = (data >> 12) & 0xFF;
	ram[addr + 3] = (data >> 16) & 0xFF;
	return AccessStatus::Ok;
}
