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

	void dump_context() const;

	private:
	auto read() -> char;

	auto token_string() const -> std::string_view;

	auto parse_integral() -> std::size_t;
	auto parse_string_literal() -> std::string_view;

	void skip_spaces();

	std::string_view                 m_source;
	std::string_view::const_iterator m_it, m_token_begin_it;

	char m_last = '\0';
};
