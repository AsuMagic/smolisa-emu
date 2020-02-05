#pragma once

#include "smol/common/opcodes.hpp"
#include "smol/common/registers.hpp"
#include "smol/common/types.hpp"

#include <array>
#include <string_view>
#include <variant>

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

struct ByteSelector
{
	bool is_upper_byte;
};

struct Immediate
{
	std::size_t value;
};

struct StringLiteral
{
	std::string_view text;
};

enum class Directive
{
	ByteOffset,
	IncludeBinaryFile,
	ImmediateLabel,
	Byte,
	Word
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
	tokens::ByteSelector,
	tokens::Immediate,
	tokens::StringLiteral,
	tokens::Directive,
	tokens::Colon,
	tokens::Newline,
	tokens::Eof>;

inline auto token_name(const Token& token) -> std::string_view
{
	return std::array{"unknown",
					  "mnemonic",
					  "register name",
					  "label",
					  "byte selector",
					  "immediate",
					  "string literal",
					  "assembler directive",
					  "colon",
					  "newline",
					  "eof"}
		.at(token.index());
}
