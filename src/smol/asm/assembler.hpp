#pragma once

#include <smol/asm/tokenizer.hpp>
#include <smol/asm/tokens.hpp>
#include <smol/common/types.hpp>
#include <smol/common/util.hpp>

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
	std::size_t      used = false;
};

class Assembler
{
	public:
	Context context;

	std::vector<LabelUsage>      label_uses;
	std::vector<LabelDefinition> label_definitions;

	std::vector<Byte> program_output;

	Tokenizer tokenizer;

	Assembler(std::string_view source);

	private:
	bool link_labels();

	void handle_label_declaration(const tokens::Label&);
	void handle_instruction(const tokens::Mnemonic&);
	void handle_select_offset(const tokens::SelectOffset&);
	void handle_binary_include(const tokens::IncludeBinaryFile&);

	RegisterId read_register_name();
	Byte       read_immediate();

	template<class... Ts>
	void visit_next(std::string_view expected, Ts&&... handlers)
	{
		const auto token = tokenizer.consume_token();

		std::visit(
			overloaded{[&]([[maybe_unused]] const auto& token) {
						   diagnostic(context, "Expected {}, got {}\n", expected, token_name(token));
						   throw std::runtime_error{"Assembler error"};
					   },
					   std::forward<Ts>(handlers)...},
			token);
	}

	template<class... Ts>
	void diagnostic(const Context& context, Ts&&... params) const
	{
		fmt::print(stderr, "{}:{}: {}", context.source_path, context.line, fmt::format(std::forward<Ts>(params)...));
	}
};
