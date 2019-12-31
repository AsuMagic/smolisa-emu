#include <cstdint>
#include <iostream>
#include <stdexcept>

int main()
{
	std::cerr << "Reading file from stdin\n";

	std::istream& in = std::cin;
	std::ostream& out = std::cout;

	std::size_t width = 0, height = 0;
	std::cin >> width >> height;

	if (width != 80 || height < 25)
	{
		std::cerr << "Can only handle 80x(25+) video, got " << width << 'x' << height << " instead\n";
		return 1;
	}

	if (height > 25)
	{
		std::cerr << "Truncating height " << height << " to 25\n";
	}

	while (in)
	{
		std::string frame, line;

		for (std::size_t y = 0; y < height; ++y)
		{
			std::getline(std::cin, line);

			if (y < 25)
			{
				frame += line;
			}
		}

		std::size_t read = 0;

		while (read < frame.size())
		{
			std::uint8_t byte = 0;

			for (std::size_t i = 0; i < 8; ++i)
			{
				byte |= (frame.at(read) == '#' ? 1 : 0) << i;
				++read;
			}

			out.put(byte);
		}
	}

	std::cerr << "Done\n";
}