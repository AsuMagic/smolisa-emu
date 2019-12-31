#pragma once

#include <smol/common/opcodes.hpp>
#include <smol/common/registers.hpp>
#include <smol/common/types.hpp>

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

struct Immediate
{
	Byte value;
};

struct SelectOffset
{
	std::size_t address;
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

inline std::string_view token_name(const Token& token)
{
	return std::array{"unknown",
					  "mnemonic",
					  "register name",
					  "label",
					  "immediate",
					  "offset selection",
					  "include binary file",
					  "colon",
					  "newline",
					  "eof"}
		.at(token.index());
}
