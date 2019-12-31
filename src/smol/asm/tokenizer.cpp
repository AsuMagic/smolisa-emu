#include "tokenizer.hpp"

Tokenizer::Tokenizer(std::string_view source) : m_source{source}, m_it{m_source.begin()} {}

Token Tokenizer::consume_token()
{
	if (!read())
	{
		return tokens::Eof{};
	}

	// Strip whitespace
	while (is_space(m_last) && read())
		;

	if (m_last == ';')
	{
		while (m_last != '\n' && read())
			;
	}

	if (is_newline(m_last))
	{
		return tokens::Newline{};
	}

	if (m_last == ':')
	{
		return tokens::Colon{};
	}

	auto token_begin_it = m_it - 1;

	auto token_string = [&]() -> std::string_view {
		return {&*token_begin_it, std::size_t(std::distance(token_begin_it, m_it))};
	};

	if (m_last == '@')
	{
		while (is_num(read()))
			;

		auto offset_string = token_string();
		offset_string.remove_prefix(1);

		// TODO: handle binary and hex
		return tokens::SelectOffset{std::stoull(std::string{offset_string})};
	}

	if (m_last == '#')
	{
		while (!is_newline(read()) && m_it != m_source.end())
			;

		auto offset_string = token_string();
		offset_string.remove_prefix(1);

		return tokens::IncludeBinaryFile{offset_string};
	}

	if (m_last == '\'')
	{
		const char ascii = read();

		if (read() == '\'')
		{
			return tokens::Immediate{Byte(ascii)};
		}

		return std::monostate{};
	}

	if (is_identifier_begin(m_last) || m_last == '$')
	{
		while (is_identifier(read()))
			;

		if (m_it != m_source.end())
		{
			// Backtrack just once
			--m_it;
		}

		auto str = token_string();

		if (const auto it = std::find_if(
				instruction_infos.begin(),
				instruction_infos.end(),
				[&](const auto& info) { return info.mnemonic == str; });
			it != instruction_infos.end())
		{
			return tokens::Mnemonic{Opcode(std::distance(instruction_infos.begin(), it))};
		}

		if (const auto it = std::find(register_infos.begin(), register_infos.end(), str); it != register_infos.end())
		{
			return tokens::RegisterReference{RegisterId(std::distance(register_infos.begin(), it))};
		}

		return tokens::Label{str};
	}

	if (is_num(m_last))
	{
		while (is_num(read()))
			;

		// TODO: handle binary and hex

		return tokens::Immediate{Byte(std::stoi(std::string{token_string()}))};
	}

	// Check one last time whether we got EOF
	if (m_it == m_source.end())
	{
		return tokens::Eof{};
	}

	return std::monostate{};
}

char Tokenizer::read()
{
	if (m_it != m_source.end())
	{
		m_last = *m_it;
		++m_it;

		return m_last;
	}

	m_last = '\0';
	return m_last;
}
