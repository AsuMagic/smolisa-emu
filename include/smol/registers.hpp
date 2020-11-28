#pragma once

#include <smol/types.hpp>
#include <string>

enum class RegisterId : Byte
{
	Ip   = 14,
	Bank = 15
};

inline std::string register_name(RegisterId id)
{
	switch (id)
	{
	case RegisterId::Ip: return "ip";
	case RegisterId::Bank: return "bank";
	default: return std::string() + 'g' + std::to_string(int(id));
	}
}
