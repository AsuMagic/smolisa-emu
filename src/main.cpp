#include "smol/memory.hpp"
#include <smol/core.hpp>
#include <smol/framebuffer/framebuffer.hpp>
#include <smol/ioutil.hpp>

#include <algorithm>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

std::string_view oopsie_woopsie()
{
	constexpr std::array<std::string_view, 4> messages = {
		"[metal pipe sound]",
		"I'm afraid I can't do that, Dave.",
		"lp0 on fire",
		"Halted and caught fire because I felt like it."
	};

	srand(time(NULL));
	return messages.at(rand() % messages.size());
}

auto main(int argc, char** argv) -> int
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
	Loads a memory dump of a smolisa machine and boots it from address 0)");
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
			"Memory initialization file '{}' ({} bytes) does not fit within system memory",
			rom_path,
			rom.size());
	}

	// Copy ROM contents to beginning of RAM
	std::copy_n(rom.begin(), std::min(rom.size(), core.mmu.ram.size()), core.mmu.ram.begin());

#ifdef SMOLISA_FRAMEBUFFER
	fmt::print(stderr, "Preparing 80x25 standard framebuffer\n");
	FrameBuffer fb;
#endif
	core.mmu.mmio_write_callback = [&](
		[[maybe_unused]] Addr addr,
		[[maybe_unused]] u32 data,
		AccessGranularity granularity
	) -> AccessStatus {
#ifdef SMOLISA_FRAMEBUFFER
		/*
		// Display retired instruction count on vsync
		if (addr == 0x0FD0)
		{
			fmt::print(stderr, ">>> retired {} instructions\n", core.retired_instructions);
		}
		*/

		// TODO: proper checks and interface, granularity

		if (!fb.set_byte(addr - fb.mmio_address, u8(data)))
		{
			return AccessStatus::ErrorMmioUnmapped;
		}
#endif
		return AccessStatus::Ok;
	};

	core.mmu.mmio_read_callback = [&](Addr addr, AccessGranularity granularity) -> std::pair<AccessStatus, u32> {
#ifdef SMOLISA_FRAMEBUFFER
		// TODO: proper checks and interface, granularity
		if (const auto v = fb.get_byte(addr - fb.mmio_address); v.has_value())
		{
			return {AccessStatus::Ok, u32(v.value())};
		}
#endif

		return {AccessStatus::ErrorMmioUnmapped, 0};
	};

	core.keepalive = [&] {
		if (fb.should_present())
		{
			fb.display();
		}
	};

	fb.display_simple_string(
		fmt::format(
			"smol2-emu [{}MiB] [{}@{:#010x}]",
			Mmu::system_memory_size / (1024 * 1024),
			rom_path,
			core.rip
		),
		0,
		24,
		FrameBuffer::normal_color
	);

	fmt::print(stderr, "Booting CPU at {:#10x}\n", core.rip);

	try
	{
		core.boot();
	}
	catch (const std::exception& e)
	{
		const std::string error = fmt::format(
			"{}{}\n{}{}\n",
			"Uncaught error: ",
			e.what(),
			oopsie_woopsie(),
			core.debug_state_multiline()
		);

#ifdef SMOLISA_FRAMEBUFFER
		fb.display_simple_string(error, 0, 1);
		fmt::print(stderr, "{}", error);
#endif
	}

#ifdef SMOLISA_FRAMEBUFFER
	while (fb.display())
	{}
#endif
}
