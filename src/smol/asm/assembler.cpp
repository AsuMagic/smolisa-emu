#include "assembler.hpp"

#include <smol/common/ioutil.hpp>
#include <smol/common/util.hpp>

Assembler::Assembler(std::string_view source) : tokenizer{source}
{
	context.source_path = "<source>";

	bool done = false;

	while (!done)
	{
		std::visit(
			overloaded{unexpected_handler("mnemonic, label declaration or any assembler directive"),
					   [this](const tokens::Mnemonic& m) { handle_instruction(m); },
					   [this](const tokens::Label& m) { handle_label_declaration(m); },
					   [this](const tokens::SelectOffset& m) { handle_select_offset(m); },
					   [this](const tokens::IncludeBinaryFile& m) { handle_binary_include(m); },
					   [&](tokens::Newline) { ++context.line; },
					   [&](tokens::Eof) { done = true; }},
			tokenizer.consume_token());
	}

	if (!link_labels())
	{
		throw std::runtime_error{"Label linking failed"};
	}
}

bool Assembler::link_labels()
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
			diagnostic(context, "Unused label '{}'\n", decl.name);
		}
	}

	for (const auto& label : label_uses)
	{
		if (!label.overriden)
		{
			diagnostic(context, "Label usage '{}' has no matching definition\n", label.name);
			fatal = true;
		}
	}

	return !fatal;
}

void Assembler::handle_label_declaration(const tokens::Label& label)
{
	std::visit(
		overloaded{unexpected_handler("colon after label declaration"),
				   [&](tokens::Colon) {
					   label_definitions.push_back({context, label.name});
				   }},
		tokenizer.consume_token());
}

void Assembler::handle_instruction(const tokens::Mnemonic& mnemonic)
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

	// TODO: just an emit function
	program_output.push_back(instruction);
	program_output.push_back(instruction >> 8);
	context.instruction_offset += 2;
}

void Assembler::handle_select_offset(const tokens::SelectOffset& select_offset)
{
	// TODO: allow setting the offset in the past
	context.instruction_offset = select_offset.address;
	program_output.resize(select_offset.address);
}

void Assembler::handle_binary_include(const tokens::IncludeBinaryFile& include)
{
	const auto content = load_file_raw(include.path);
	program_output.insert(program_output.end(), content.begin(), content.end());
	context.instruction_offset += content.size();
}

RegisterId Assembler::read_register_name()
{
	RegisterId id; // TODO: this is terrible

	std::visit(
		overloaded{unexpected_handler("register name"), [&](tokens::RegisterReference reg) { id = reg.id; }},
		tokenizer.consume_token());

	return id;
}

Byte Assembler::read_immediate()
{
	Byte byte = 0xFF;

	std::visit(
		overloaded{unexpected_handler("immediate value or label"),
				   [&](tokens::Label label) {
					   label_uses.push_back({context, label.name, 0});
				   },
				   [&](tokens::Immediate immediate) { byte = immediate.value; }},
		tokenizer.consume_token());

	return byte;
}
