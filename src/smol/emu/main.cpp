#include <smol/emu/core.hpp>
#include <smol/emu/framebuffer/framebuffer.hpp>

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
			"This means that data may be copied over to pages the software has to enable to get access to.\n",
			rom_path,
			rom.size());
	}

	// Copy ROM contents to beginning of RAM
	std::copy_n(rom.begin(), std::min(rom.size(), core.mmu.ram.size()), core.mmu.ram.begin());

	fmt::print(stderr, "Preparing 80x25 standard framebuffer\n");
	FrameBuffer fb;

	core.mmu.mmio_write_callback = [&](Addr addr, Byte byte) {
		if (!fb.set_byte(addr, byte))
		{
			throw std::runtime_error{fmt::format("Illegal MMIO write @{:#06x}: {:#04x}\n", addr, byte)};
		}
	};

	core.mmu.mmio_read_callback = [&](Addr addr) -> Byte {
		try
		{
			return fb.get_byte(addr).value();
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error{fmt::format("Illegal MMIO read @{:#06x}\n", addr)};
		}
	};

	core.instruction_pointer = 0x0000;
	fmt::print(stderr, "Booting CPU at {:#06x}\n", core.instruction_pointer);

	try
	{
		core.boot();
	}
	catch (const std::exception& e)
	{
		const std::string error = fmt::format("Emulator caught fire: {}{}\n", e.what(), core.debug_state());

		fb.display_simple_string(error, 0, 1);
		fmt::print(stderr, "{}", error);
	}

	while (fb.display())
		;
}
