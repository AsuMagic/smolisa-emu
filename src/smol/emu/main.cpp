#include <smol/emu/core.hpp>

#include <algorithm>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

std::vector<char> load_file_raw(std::string_view path)
{
	std::ifstream file{std::string{path}, std::ios::binary | std::ios::ate};

	if (!file)
	{
		throw std::runtime_error{"Failed to load ROM from file"};
	}

	// We are already seeking to the end because of `std::ios::ate`
	const std::size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> ret(size);
	file.read(ret.data(), size);

	return ret;
}

int main(int argc, char** argv)
{
	const std::vector<std::string_view> args(argv + 1, argv + argc);

	if (args.size() != 1)
	{
		fmt::print(stderr, "Syntax: ./smolisa-emu <ram_boot_dump>\n");
		return 1;
	}

	if (args[0] == "-h" || args[0] == "--help")
	{
		fmt::print(
			stderr,
			R"(Syntax: ./smolisa-emu <ram_boot_dump>
	Loads a memory dump of a smolisa machine and boots it from address 0x0000)");
		return 1;
	}

	const auto& rom_path = args[0];

	const auto rom = load_file_raw(rom_path);

	Core core;

	if (rom.size() > core.mmu.ram.size())
	{
		fmt::print(
			stderr,
			"Memory initialization file '{}' ({} bytes) does not fit in the emulated machine RAM ({} bytes), "
			"truncating\n",
			rom_path,
			rom.size(),
			core.mmu.ram.size());
	}
	else if (rom.size() > Mmu::system_memory_size)
	{
		fmt::print(
			stderr,
			"Memory initialization file '{}' ({} bytes) does not fit within the 8KiB of system memory.\n"
			"This means that data may be copied over to pages the software has to enable to get access to.",
			rom_path,
			rom.size());
	}

	// Copy ROM contents to beginning of RAM
	std::copy_n(rom.begin(), std::min(rom.size(), core.mmu.ram.size()), core.mmu.ram.begin());

	core.mmu.mmio_write_callback
		= [](Addr addr, Byte byte) { fmt::print(stderr, "MMIO write @{:#06x}: {:#04x}\n", addr, byte); };

	core.instruction_pointer = 0x0000;
	core.boot();
}
