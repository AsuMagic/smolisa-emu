#include <cstdint>
#include <iostream>
#include <stdexcept>

constexpr std::size_t target_width = 80, target_height = 25;

auto main() -> int
{
	std::cerr << "Reading file from stdin\n";

	std::istream& in  = std::cin;
	std::ostream& out = std::cout;

	std::size_t width  = 0;
	std::size_t height = 0;
	std::cin >> width >> height;

	if (width != target_width || height < target_height)
	{
		std::cerr << "Can only handle 80x(25+) video, got " << width << 'x' << height << " instead\n";
		return 1;
	}

	if (height > target_height)
	{
		std::cerr << "Truncating height " << height << " to 25\n";
	}

	while (in)
	{
		std::string frame;
		std::string line;

		for (std::size_t y = 0; y < height; ++y)
		{
			std::getline(std::cin, line);

			if (y < target_height)
			{
				frame += line;
			}
		}

		std::size_t read = 0;

		while (read < frame.size())
		{
			std::uint8_t byte = 0;

			constexpr std::size_t pixels_per_byte = 8;
			for (std::size_t i = 0; i < pixels_per_byte; ++i)
			{
				if (frame.at(read) == '#')
				{
					byte |= 1U << i;
				}

				++read;
			}

			out.put(byte);
		}
	}

	std::cerr << "Done\n";
}
