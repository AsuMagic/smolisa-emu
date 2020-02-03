#include "smol/asm/assembler.hpp"

#include "smol/common/ioutil.hpp"
#include "smol/common/masks.hpp"

Assembler::Assembler(std::string_view source) : tokenizer{source}
{
	context.source_path = "<source>";

	bool done = false;

	while (!done)
	{
		visit_next_token(
			"mnemonic, label declaration or any assembler directive",
			[this](tokens::Mnemonic m) { handle_instruction(m); },
			[this](tokens::Label m) { handle_label_declaration(m); },
			[this](tokens::Directive m) { handle_directive(m); },
			[&]([[maybe_unused]] tokens::Newline) { ++context.line; },
			[&]([[maybe_unused]] tokens::Eof) { done = true; });
	}

	if (!link_labels())
	{
		throw std::runtime_error{"Label linking failed"};
	}
}

void Assembler::emit(Byte byte)
{
	program_output.push_back(byte);
	++context.instruction_offset;
}

auto Assembler::link_labels() -> bool
{
	bool fatal = false;

	for (auto& decl : label_definitions)
	{
		for (auto& label : label_uses)
		{
			if (label.name == decl.name)
			{
				// Override the imm8
				program_output.at(label.context.instruction_offset + 1)
					= decl.context.instruction_offset >> label.bit_offset;

				label.overriden = true;
				decl.used       = true;
			}
		}
	}

	for (const auto& decl : label_definitions)
	{
		if (!decl.used)
		{
			diagnostic(decl.context, "Unused label '{}'\n", decl.name);
		}
	}

	for (const auto& label : label_uses)
	{
		if (!label.overriden)
		{
			diagnostic(label.context, "Label usage '{}' has no matching definition\n", label.name);
			fatal = true;
		}
	}

	return !fatal;
}

void Assembler::expect_newline()
{
	visit_next_token(
		"newline", [&]([[maybe_unused]] tokens::Newline) {}, [&]([[maybe_unused]] tokens::Eof) {});
}

void Assembler::handle_label_declaration(tokens::Label label)
{
	visit_next_token("colon after label declaration", [&]([[maybe_unused]] tokens::Colon) {
		label_definitions.push_back({context, label.name});
	});

	expect_newline();
}

void Assembler::handle_instruction(tokens::Mnemonic mnemonic)
{
	const auto& info = instruction_infos[Byte(mnemonic.opcode)];

	Word instruction = 0;
	instruction |= Byte(mnemonic.opcode);

	switch (info.type)
	{
	case InstructionType::R8R8R8:
	{
		if (info.useful_parameters >= 1)
		{
			instruction |= Byte(read_register_name()) << 4;
		}

		if (info.useful_parameters >= 2)
		{
			instruction |= Byte(read_register_name()) << 8;
		}

		if (info.useful_parameters >= 3)
		{
			instruction |= Byte(read_register_name()) << 12;
		}

		break;
	}

	case InstructionType::R8Imm8:
	{
		if (info.useful_parameters >= 1)
		{
			instruction |= Byte(read_register_name()) << 4;
		}

		if (info.useful_parameters >= 2)
		{
			instruction |= Byte(read_immediate()) << 8;
		}

		break;
	}
	}

	emit(Byte(instruction));
	emit(Byte(instruction >> 8));

	expect_newline();
}

void Assembler::handle_directive(tokens::Directive directive)
{
	switch (directive)
	{
	case tokens::Directive::ByteOffset:
	{
		const auto offset = visit_next_token<std::size_t>(
			"byte offset immediate value", [](tokens::Immediate immediate) { return immediate.value; });
		handle_select_offset(offset);
		break;
	}

	case tokens::Directive::IncludeBinaryFile:
	{
		const auto text = visit_next_token<std::string_view>(
			"binary file path string", [](tokens::StringLiteral literal) { return literal.text; });

		handle_binary_include(text);
		break;
	}
	}
}

void Assembler::handle_select_offset(std::size_t select_offset)
{
	// TODO: allow setting the offset in the past
	context.instruction_offset = select_offset;
	program_output.resize(select_offset);

	expect_newline();
}

void Assembler::handle_binary_include(std::string_view include_path)
{
	const auto content = load_file_raw(include_path);
	program_output.insert(program_output.end(), content.begin(), content.end());
	context.instruction_offset += content.size();

	expect_newline();
}

auto Assembler::read_register_name() -> RegisterId
{
	return visit_next_token<RegisterId>("register name", [&](tokens::RegisterReference reg) { return reg.id; });
}

auto Assembler::read_immediate() -> Byte
{
	Byte byte = 0xFF;

	visit_next_token(
		"immediate value or label",
		[&](tokens::Label label) {
			const auto offset_token = visit_next_token<tokens::ByteSelector>(
				"byte selector '~'", [](tokens::ByteSelector offset) { return offset; });
			label_uses.push_back({context, label.name, offset_token.is_upper_byte ? 8u : 0u});
		},
		[&](tokens::Immediate immediate) { byte = Byte(immediate.value); });

	return byte;
}
