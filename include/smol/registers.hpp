#pragma once

#include <smol/types.hpp>
#include <string>

enum class RegisterId : u8
{
	RRET = 13,
	RPL = 14,
	RPS = 15
};

inline auto register_name(RegisterId id) -> std::string
{
	switch (id)
	{
	case RegisterId::RRET: return "rret";
	case RegisterId::RPL: return "rpl";
	case RegisterId::RPS: return "rps";
	default: return std::string() + 'r' + std::to_string(int(id));
	}
}
