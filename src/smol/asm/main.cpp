#include <algorithm>
#include <array>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <smol/common/opcodes.hpp>
#include <smol/common/registers.hpp>

#include <smol/asm/instructioninfo.hpp>
#include <smol/asm/registerinfo.hpp>

template<class... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

namespace tokens
{
struct Mnemonic
{
	Opcode opcode;
};

struct RegisterReference
{
	RegisterId id;
};

struct Colon
{};

struct Label
{
	std::string_view name;
};

struct Immediate
{
	Byte value;
};

struct SelectOffset
{
	std::size_t addr;
};

struct IncludeBinaryFile
{
	std::string_view path;
};

struct Newline
{};

struct Eof
{};
} // namespace tokens

using Token = std::variant<
	std::monostate,
	tokens::Mnemonic,
	tokens::RegisterReference,
	tokens::Label,
	tokens::Immediate,
	tokens::SelectOffset,
	tokens::IncludeBinaryFile,
	tokens::Colon,
	tokens::Newline,
	tokens::Eof>;

constexpr bool is_space(char c) { return c == ' ' || c == '\t'; }
constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr bool is_num(char c) { return c >= '0' && c <= '9'; }
constexpr bool is_newline(char c) { return c == '\n'; }

constexpr bool is_identifier_begin(char c) { return is_alpha(c) || c == '_'; }
constexpr bool is_identifier(char c) { return is_identifier_begin(c) || is_num(c); }

class Tokenizer
{
	public:
	Tokenizer(std::string_view source) : m_source{source}, m_it{m_source.begin()} {}

	Token consume_token()
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

			if (const auto it = std::find(register_infos.begin(), register_infos.end(), str);
				it != register_infos.end())
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

	private:
	char read()
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

	std::string_view                 m_source;
	std::string_view::const_iterator m_it;

	char m_last = '\0';
};

struct Context
{
	std::size_t line = 1;
};

struct LabelUsage
{
	Context          context;
	std::string_view name;
	std::size_t      byte_to_override_offset; //!< Offset of the byte to override in the program
	std::size_t bit_offset = 0; //!< Offset of the byte to copy (e.g. 8 to get the 8 upper bits of a 16-bit address)
	bool        overriden  = false;
};

struct LabelDefinition
{
	Context          context;
	std::string_view name;
	std::size_t      address;
	std::size_t      used = false;
};

std::vector<char> load_file_raw(std::string_view path)
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

int main(int argc, char** argv)
{
	const std::vector<std::string_view> args(argv + 1, argv + argc);

	if (args.size() != 1)
	{
		fmt::print(stderr, "Syntax: ./smolisa-as <source>\nBinaries are written on stdout\n");
		return 1;
	}

	// TODO: ugly af
	auto      source = load_file_raw(args[0]);
	Tokenizer tokenizer{source.data()};

	std::vector<LabelUsage>      label_uses;
	std::vector<LabelDefinition> label_definitions;
	std::vector<Byte>            output;

	std::size_t current_offset = 0;

	Context context;

	/*const auto name_of = [](const Token& token) -> std::string_view {
		return std::array{
			"unknown", "mnemonic", "register", "label", "immediate", "colon", "newline", "eof"}[token.index()];
	};*/

	const auto unexpected_fallback
		= [&](auto) { throw std::runtime_error{fmt::format("Unexpected token, current offset = {}", current_offset)}; };

	const auto handle_label_declaration = [&](tokens::Label decl) {
		std::visit(
			overloaded{unexpected_fallback,
					   [&](tokens::Colon) {
						   label_definitions.push_back({context, decl.name, current_offset});
					   }},
			tokenizer.consume_token());
	};

	const auto read_register = [&]() {
		RegisterId id; // TODO: this is terrible

		std::visit(
			overloaded{unexpected_fallback, [&](tokens::RegisterReference reg) { id = reg.id; }},
			tokenizer.consume_token());

		return id;
	};

	const auto read_immediate = [&]() {
		Byte byte = 0xFF;

		std::visit(
			overloaded{unexpected_fallback,
					   [&](tokens::Label label) {
						   label_uses.push_back({context, label.name, current_offset + 1, 0});
					   },
					   [&](tokens::Immediate immediate) { byte = immediate.value; }},
			tokenizer.consume_token());

		return byte;
	};

	const auto handle_instruction = [&](tokens::Mnemonic mnemonic) {
		const auto& info = instruction_infos[Byte(mnemonic.opcode)];

		Word instruction = 0;
		instruction |= Byte(mnemonic.opcode);

		switch (info.type)
		{
		case InstructionType::R8R8R8:
		{
			if (info.useful_parameters >= 1)
			{
				instruction |= Byte(read_register()) << 4;
			}

			if (info.useful_parameters >= 2)
			{
				instruction |= Byte(read_register()) << 8;
			}

			if (info.useful_parameters >= 3)
			{
				instruction |= Byte(read_register()) << 12;
			}

			break;
		}

		case InstructionType::R8Imm8:
		{
			if (info.useful_parameters >= 1)
			{
				instruction |= Byte(read_register()) << 4;
			}

			if (info.useful_parameters >= 2)
			{
				instruction |= Byte(read_immediate()) << 8;
			}

			break;
		}
		}

		// TODO: just an emit function
		output.push_back(instruction);
		output.push_back(instruction >> 8);
		current_offset += 2;
	};

	// TODO: allow setting the offset in the past
	const auto handle_select_offset = [&](tokens::SelectOffset offset) {
		current_offset = offset.addr;
		output.resize(current_offset);
	};

	const auto handle_binary_include = [&](tokens::IncludeBinaryFile include) {
		const auto content = load_file_raw(include.path);
		output.insert(output.end(), content.begin(), content.end());
		current_offset += content.size();
	};

	// hax
	bool done  = false;
	bool fatal = false;

	while (!done)
	{
		std::visit(
			overloaded{unexpected_fallback,
					   handle_instruction,
					   handle_label_declaration,
					   handle_select_offset,
					   handle_binary_include,
					   [&](tokens::Newline) { ++context.line; },
					   [&](tokens::Eof) { done = true; }},
			tokenizer.consume_token());
	}

	const auto error = [&](Context context, const auto&... params) {
		fmt::print(stderr, "{}:{}: {}", args[0], context.line, fmt::format(std::forward<decltype(params)>(params)...));
	};

	for (auto& decl : label_definitions)
	{
		for (auto& label : label_uses)
		{
			if (label.name == decl.name)
			{
				output[label.byte_to_override_offset] = decl.address >> label.bit_offset;
				label.overriden                       = true;
				decl.used                             = true;
			}
		}
	}

	for (const auto& decl : label_definitions)
	{
		if (!decl.used)
		{
			error(context, "Unused label '{}'\n", decl.name);
		}
	}

	for (const auto& label : label_uses)
	{
		if (!label.overriden)
		{
			error(context, "Label usage '{}' has no matching definition\n", label.name);
			fatal = true;
		}
	}

	if (fatal)
	{
		fmt::print(stderr, "Quitting because of past errors\n");
		return 1;
	}

	for (Byte b : output)
	{
		std::cout.put(b);
	}

	// TODO: allow to put arbitrary bytes
	// TODO: less shit syntax errors (use name_of at least or something, wrap std::visit and overloaded)
	// TODO: split that crap up

#warning TODO allowing to load low and high bits of address!!
}
