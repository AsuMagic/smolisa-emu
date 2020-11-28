#pragma once

#include <string_view>
#include <vector>

auto load_file_raw(std::string_view path) -> std::vector<char>;
