#pragma once

#include "smol/asm/charutil.hpp"
#include "smol/asm/instructioninfo.hpp"
#include "smol/asm/registerinfo.hpp"
#include "smol/asm/tokens.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

class Tokenizer
{
	public:
	explicit Tokenizer(std::string_view source);

	auto consume_token() -> Token;

	private:
	auto read() -> char;

	std::string_view                 m_source;
	std::string_view::const_iterator m_it;

	char m_last = '\0';
};
