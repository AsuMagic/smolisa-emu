#pragma once

#include <vector>
#include <string_view>
#include <fstream>

inline std::vector<char> load_file_raw(std::string_view path)
{
	std::ifstream file{std::string{path}, std::ios::binary | std::ios::ate};

	if (!file)
	{
		throw std::runtime_error{"Failed to load file"};
	}

	// We are already seeking to the end because of `std::ios::ate`
	const std::size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> ret(size);
	file.read(ret.data(), size);

	return ret;
}
