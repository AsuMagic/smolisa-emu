#include "smol/asm/assembler.hpp"
#include "smol/common/ioutil.hpp"

#include <fmt/core.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

auto main(int argc, char** argv) -> int
{
	try
	{
		const std::vector<std::string_view> args(argv + 1, argv + argc);

		if (args.size() != 1)
		{
			fmt::print(stderr, "Syntax: ./smolisa-as <source>\nBinaries are written on stdout\n");
			return 1;
		}

		const auto source_path = args[0];
		const auto source      = load_file_raw(source_path);
		Assembler  assembler{std::string_view{source.data(), source.size()}};

		for (Byte byte : assembler.program_output)
		{
			std::cout.put(byte);
		}

		// TODO: allow to put arbitrary bytes
	}
	catch (const std::exception& e)
	{
		fmt::print(stderr, "Assembler exited: {}\n", e.what());
		return 1;
	}
}
