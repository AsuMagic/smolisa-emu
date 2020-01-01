#pragma once

#include "smol/asm/tokenizer.hpp"
#include "smol/asm/tokens.hpp"
#include "smol/common/types.hpp"
#include "smol/common/util.hpp"

#include <cstddef>
#include <fmt/core.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct Context
{
	std::string source_path;
	std::size_t line               = 1;
	std::size_t instruction_offset = 0;
};

struct LabelUsage
{
	Context          context;
	std::string_view name;
	std::size_t bit_offset = 0; //!< Offset of the byte to copy (e.g. 8 to get the 8 upper bits of a 16-bit address)
	bool        overriden  = false;
};

struct LabelDefinition
{
	Context          context;
	std::string_view name;
	bool             used = false;
};

class Assembler
{
	public:
	Context context;

	std::vector<LabelUsage>      label_uses;
	std::vector<LabelDefinition> label_definitions;

	std::vector<Byte> program_output;

	Tokenizer tokenizer;

	explicit Assembler(std::string_view source);

	void emit(Byte byte);

	private:
	auto link_labels() -> bool;

	void handle_label_declaration(tokens::Label label);
	void handle_instruction(tokens::Mnemonic mnemonic);
	void handle_select_offset(tokens::SelectOffset select_offset);
	void handle_binary_include(tokens::IncludeBinaryFile include);

	auto read_register_name() -> RegisterId;
	auto read_immediate() -> Byte;

	template<class R = void, class... Ts>
	auto visit_next_token(std::string_view expected, Ts&&... handlers) -> R
	{
		const auto token = tokenizer.consume_token();

		const auto unexpected_handler = [&]([[maybe_unused]] const auto& token) -> R {
			diagnostic(context, "Expected {}, got {}\n", expected, token_name(token));
			throw std::runtime_error{"Assembler error"};
		};

		return std::visit<R>(overloaded{unexpected_handler, std::forward<Ts>(handlers)...}, token);
	}

	template<class... Ts>
	void diagnostic(const Context& context, Ts&&... params) const
	{
		fmt::print(stderr, "{}:{}: {}", context.source_path, context.line, fmt::format(std::forward<Ts>(params)...));
	}
};
