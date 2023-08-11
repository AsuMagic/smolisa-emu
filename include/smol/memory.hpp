#pragma once

#include <smol/types.hpp>

#include <functional>
#include <vector>
#include <array>
#include <string_view>

enum class AccessStatus
{
	Ok,
	ErrorUnmapped,
	ErrorMisaligned,
	ErrorMmioGranularity,
	ErrorMmioPeripheralError,
	ErrorMmioUnmapped,
	Count
};

static constexpr std::array<std::string_view, int(AccessStatus::Count)> access_status_strings = {
	"Valid memory access",
	"Unmapped memory access",
	"Misaligned memory access",
	"Illegal granularity for MMIO address",
	"Illegal address for MMIO peripheral",
	"Unmapped MMIO address"
};

enum class AccessGranularity
{
	U8,
	U16,
	U32
};

struct Mmu
{
	static constexpr auto address_space_size = (std::uint64_t(1) << (sizeof(Word) * 8));
	static constexpr auto system_memory_size = std::uint64_t(0x1000'0000);

	static constexpr auto mmio_start_address = std::uint32_t(0xF000'0000);

	static constexpr auto mmio_address(Addr real_address) -> Addr { return real_address - mmio_start_address; }

	std::vector<u8> ram;

	std::function<std::pair<AccessStatus, u32>(Addr, AccessGranularity)>       mmio_read_callback;
	std::function<AccessStatus(Addr, u32, AccessGranularity)>                  mmio_write_callback;

	explicit Mmu();

	[[nodiscard]] auto is_mmio(Addr addr) const -> bool { return addr >= mmio_start_address; };
	[[nodiscard]] auto is_mapped(Addr addr) const -> bool { return is_mmio(addr) || addr < system_memory_size; }

	[[nodiscard]] auto get_u8(Addr addr) const -> std::pair<AccessStatus, u8>;
	auto               set_u8(Addr addr, u8 data) -> AccessStatus;

	[[nodiscard]] auto get_u16(Addr addr) const -> std::pair<AccessStatus, u16>;
	auto               set_u16(Addr addr, u16 data) -> AccessStatus;

	[[nodiscard]] auto get_u32(Addr addr) const -> std::pair<AccessStatus, u32>;
	auto               set_u32(Addr addr, u32 data) -> AccessStatus;
};
