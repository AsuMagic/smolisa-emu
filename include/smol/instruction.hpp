#pragma once

#include <smol/util.hpp>
#include <smol/masks.hpp>
#include <smol/registers.hpp>
#include <smol/types.hpp>

#include <array>
#include <type_traits>
#include <variant>
#include <fmt/core.h>

template<class T>
auto sext(T x, std::size_t bit_count)
{
	int mask = std::uint64_t(1) << (bit_count - 1);
	return (x ^ mask) - mask;
}

template<class T>
T bits(Instruction ins, std::size_t first_bit, std::size_t bit_count)
{
	const std::uint64_t mask = (std::uint64_t(1) << bit_count) - 1;
	std::uint64_t extracted = (std::uint64_t(ins) >> first_bit) & mask;

	if constexpr (std::is_signed_v<T>)
	{
		return sext(extracted, bit_count);
	}

	return T(extracted);
}

namespace decoders
{

using R = RegisterId;

void r4(Instruction ins, RegisterId& r)
{
	r = bits<R>(ins, 0, 4);
}

void r4r4(Instruction ins, RegisterId& a, RegisterId& b)
{
	a = bits<R>(ins, 0, 4);
	b = bits<R>(ins, 4, 4);
}

template<bool Signed>
void r4r4e16(Instruction ins, RegisterId& a, RegisterId& b, std::conditional_t<Signed, s32, u32>& imm)
{
	r4r4(ins, a, b);
	if constexpr (Signed)
	{
		imm = bits<s32>(ins, 16, 16);
	}
	else
	{
		imm = bits<u32>(ins, 16, 16);
	}
}

template<bool Signed>
void rh2r2i6(Instruction ins, RegisterId& a, RegisterId& b, std::conditional_t<Signed, s32, u32>& imm)
{
	a = R((bits<u32>(ins, 0, 2)) | 0b1000);
	b = R(bits<u32>(ins, 2, 2));

	if constexpr (Signed)
	{
		imm = bits<s32>(ins, 4, 6);
	}
	else
	{
		imm = bits<u32>(ins, 4, 6);
	}
}

template<bool Signed>
void r4i4(Instruction ins, RegisterId& r, std::conditional_t<Signed, s32, u32>& imm)
{
	r = bits<R>(ins, 0, 4);
	if constexpr (Signed)
	{
		imm = bits<s32>(ins, 4, 4);
	}
	else
	{
		imm = bits<u32>(ins, 4, 4);
	}
}

template<bool Signed>
void r4i5(Instruction ins, RegisterId& r, std::conditional_t<Signed, s32, u32>& imm)
{
	r = bits<R>(ins, 0, 4);
	if constexpr (Signed)
	{
		imm = bits<s32>(ins, 4, 5);
	}
	else
	{
		imm = bits<u32>(ins, 4, 5);
	}
}

template<bool Signed>
void r4i8(Instruction ins, RegisterId& r, std::conditional_t<Signed, s32, u32>& imm)
{
	r = bits<R>(ins, 0, 4);
	if constexpr (Signed)
	{
		imm = bits<s32>(ins, 4, 8);
	}
	else
	{
		imm = bits<u32>(ins, 4, 8);
	}
}

template<bool Signed>
void r4i8e16(Instruction ins, RegisterId& r, std::conditional_t<Signed, s32, u32>& imm)
{
	r = bits<R>(ins, 0, 4);
	u32 unsigned_imm = 
		bits<u32>(ins, 4, 8)
		| bits<u32>(ins, 16, 16) << 8;
	imm = sext(unsigned_imm, 24);
}

template<bool Signed>
void i28(Instruction ins, std::conditional_t<Signed, s32, u32>& imm)
{
	u32 unsigned_imm = 
		bits<u32>(ins, 0, 12)
		| bits<u32>(ins, 16, 16) << 12;
	imm = sext(unsigned_imm, 28);
}

template<bool Signed>
void i12(Instruction ins, std::conditional_t<Signed, s32, u32>& imm)
{
	imm = sext(bits<u32>(ins, 0, 12), 12);
}

} // namespace decoders

namespace formats
{

using R = RegisterId;

struct MemLoad
{
	R addr, dst;
	explicit MemLoad(Instruction ins) { decoders::r4r4(ins, addr, dst); }
	static constexpr std::size_t length = 2;
};

struct RegLoad
{
	R src, dst;
	explicit RegLoad(Instruction ins) { decoders::r4r4(ins, src, dst); }
	static constexpr std::size_t length = 2;
};

struct MemLoadWideOffset
{
	R base_addr, dst;
	s32 offset;
	explicit MemLoadWideOffset(Instruction ins) { decoders::r4r4e16<true>(ins, base_addr, dst, offset); }
	static constexpr std::size_t length = 4;
};

struct MemLoadShortOffset
{
	R base_addr, dst;
	u32 offset;
	explicit MemLoadShortOffset(Instruction ins) { decoders::rh2r2i6<false>(ins, base_addr, dst, offset); }
	static constexpr std::size_t length = 2;
};

struct ImmByteLoad
{
	R dst;
	s32 imm;
	explicit ImmByteLoad(Instruction ins) { decoders::r4i8<true>(ins, dst, imm); }
	static constexpr std::size_t length = 2;
};

struct ImmI24Load
{
	R dst;
	s32 imm;
	explicit ImmI24Load(Instruction ins) { decoders::r4i8e16<true>(ins, dst, imm); }
	static constexpr std::size_t length = 4;
};

struct MemStore
{
	R addr, src;
	explicit MemStore(Instruction ins) { decoders::r4r4(ins, addr, src); }
	static constexpr std::size_t length = 2;
};

struct MemStoreWideOffset
{
	R base_addr, src;
	s32 offset;
	explicit MemStoreWideOffset(Instruction ins) { decoders::r4r4e16<true>(ins, base_addr, src, offset); }
	static constexpr std::size_t length = 4;
};

struct MemStoreShortOffset
{
	R base_addr, src;
	u32 offset;
	explicit MemStoreShortOffset(Instruction ins) { decoders::rh2r2i6<false>(ins, base_addr, src, offset); }
	static constexpr std::size_t length = 2;
};

struct StackPush
{
	R src;
	explicit StackPush(Instruction ins) { decoders::r4(ins, src); }
	static constexpr std::size_t length = 2;
};

struct NoArg
{
	explicit NoArg(Instruction _ins) {}
	static constexpr std::size_t length = 2;
};

struct TestRegReg
{
	R a, b;
	explicit TestRegReg(Instruction ins) { decoders::r4r4(ins, a, b); }
	static constexpr std::size_t length = 2;
};

struct TestRegI4
{
	R a;
	s32 b;
	explicit TestRegI4(Instruction ins) { decoders::r4i4<true>(ins, a, b); }
	static constexpr std::size_t length = 2;
};

struct TestReg
{
	R a;
	explicit TestReg(Instruction ins) { decoders::r4(ins, a); }
	static constexpr std::size_t length = 2;
};

struct PoolLoad
{
	R dst;
	u32 offset;
	explicit PoolLoad(Instruction ins) { decoders::r4i8<false>(ins, dst, offset); }
	static constexpr std::size_t length = 2;
};

struct JumpReg
{
	R target;
	explicit JumpReg(Instruction ins) { decoders::r4(ins, target); }
	static constexpr std::size_t length = 2;
};

struct JumpLinkReg
{
	R target, dst;
	explicit JumpLinkReg(Instruction ins) { decoders::r4r4(ins, target, dst); }
	static constexpr std::size_t length = 2;
};

struct JumpLinkI28
{
	s32 relative_target;
	explicit JumpLinkI28(Instruction ins) { decoders::i28<true>(ins, relative_target); }
	static constexpr std::size_t length = 4;
};

struct JumpI12
{
	s32 relative_target;
	explicit JumpI12(Instruction ins) { decoders::i12<true>(ins, relative_target); }
	static constexpr std::size_t length = 2;
};

struct ALURegReg
{
	R a_dst, b;
	explicit ALURegReg(Instruction ins) { decoders::r4r4(ins, a_dst, b); }
	static constexpr std::size_t length = 2;
};

struct ALURegS4
{
	R a_dst;
	s32 b;
	explicit ALURegS4(Instruction ins) { decoders::r4i4<true>(ins, a_dst, b); }
	static constexpr std::size_t length = 2;
};

struct ALURegS5
{
	R a_dst;
	s32 b;
	explicit ALURegS5(Instruction ins) { decoders::r4i5<true>(ins, a_dst, b); }
	static constexpr std::size_t length = 2;
};

struct ALUWideAdd
{
	R dst, a;
	s32 b;
	explicit ALUWideAdd(Instruction ins) { decoders::r4r4e16<true>(ins, dst, a, b); }
	static constexpr std::size_t length = 4;
};

} // namespace formats

namespace insns
{

struct L8 : formats::MemLoad
{
	using formats::MemLoad::MemLoad;
};

struct L16 : formats::MemLoad
{
	using formats::MemLoad::MemLoad;
};

struct L32 : formats::MemLoad
{
	using formats::MemLoad::MemLoad;
};

struct CLR : formats::RegLoad
{
	using formats::RegLoad::RegLoad;
};

struct L8OW : formats::MemLoadWideOffset
{
	using formats::MemLoadWideOffset::MemLoadWideOffset;
};

struct L16OW : formats::MemLoadWideOffset
{
	using formats::MemLoadWideOffset::MemLoadWideOffset;
};

struct L32OW : formats::MemLoadWideOffset
{
	using formats::MemLoadWideOffset::MemLoadWideOffset;
};

struct LR : formats::RegLoad
{
	using formats::RegLoad::RegLoad;
};

struct LS8 : formats::MemLoad
{
	using formats::MemLoad::MemLoad;
};

struct LS16 : formats::MemLoad
{
	using formats::MemLoad::MemLoad;
};

struct LS8OW : formats::MemLoadWideOffset
{
	using formats::MemLoadWideOffset::MemLoadWideOffset;
};

struct LS16OW : formats::MemLoadWideOffset
{
	using formats::MemLoadWideOffset::MemLoadWideOffset;
};

struct L8O : formats::MemLoadShortOffset
{
	using formats::MemLoadShortOffset::MemLoadShortOffset;
};

struct L16O : formats::MemLoadShortOffset
{
	using formats::MemLoadShortOffset::MemLoadShortOffset;
};

struct L32O : formats::MemLoadShortOffset
{
	using formats::MemLoadShortOffset::MemLoadShortOffset;
};

struct LS8O : formats::MemLoadShortOffset
{
	using formats::MemLoadShortOffset::MemLoadShortOffset;
};

struct LS16O : formats::MemLoadShortOffset
{
	using formats::MemLoadShortOffset::MemLoadShortOffset;
};

struct LSI : formats::ImmByteLoad
{
	using formats::ImmByteLoad::ImmByteLoad;
};

struct LSIH : formats::ImmByteLoad
{
	using formats::ImmByteLoad::ImmByteLoad;
};

struct LSIW : formats::ImmI24Load
{
	using formats::ImmI24Load::ImmI24Load;
};

struct LIPREL : formats::ImmI24Load
{
	using formats::ImmI24Load::ImmI24Load;
};

struct S8 : formats::MemStore
{
	using formats::MemStore::MemStore;
};

struct S16 : formats::MemStore
{
	using formats::MemStore::MemStore;
};

struct S32 : formats::MemStore
{
	using formats::MemStore::MemStore;
};

struct PUSH : formats::StackPush
{
	using formats::StackPush::StackPush;
};

struct S8OW : formats::MemStoreWideOffset
{
	using formats::MemStoreWideOffset::MemStoreWideOffset;
};

struct S16OW : formats::MemStoreWideOffset
{
	using formats::MemStoreWideOffset::MemStoreWideOffset;
};

struct S32OW : formats::MemStoreWideOffset
{
	using formats::MemStoreWideOffset::MemStoreWideOffset;
};

struct BRK : formats::NoArg
{
	using formats::NoArg::NoArg;
};

struct S8O : formats::MemStoreShortOffset
{
	using formats::MemStoreShortOffset::MemStoreShortOffset;
};

struct S16O : formats::MemStoreShortOffset
{
	using formats::MemStoreShortOffset::MemStoreShortOffset;
};

struct S32O : formats::MemStoreShortOffset
{
	using formats::MemStoreShortOffset::MemStoreShortOffset;
};

struct TLTU : formats::TestRegReg
{
	using formats::TestRegReg::TestRegReg;
};

struct TLTS : formats::TestRegReg
{
	using formats::TestRegReg::TestRegReg;
};

struct TGEU : formats::TestRegReg
{
	using formats::TestRegReg::TestRegReg;
};

struct TGES : formats::TestRegReg
{
	using formats::TestRegReg::TestRegReg;
};

struct TE : formats::TestRegReg
{
	using formats::TestRegReg::TestRegReg;
};

struct TNE : formats::TestRegReg
{
	using formats::TestRegReg::TestRegReg;
};

struct TLTSI : formats::TestRegI4
{
	using formats::TestRegI4::TestRegI4;
};

struct TGESI : formats::TestRegI4
{
	using formats::TestRegI4::TestRegI4;
};

struct TEI : formats::TestRegI4
{
	using formats::TestRegI4::TestRegI4;
};

struct TNEI : formats::TestRegI4
{
	using formats::TestRegI4::TestRegI4;
};

struct TBZ : formats::TestReg
{
	using formats::TestReg::TestReg;
};

struct PLL32 : formats::PoolLoad
{
	using formats::PoolLoad::PoolLoad;
};

struct J : formats::JumpReg
{
	using formats::JumpReg::JumpReg;
};

struct CJ : formats::JumpReg
{
	using formats::JumpReg::JumpReg;
};

struct JAL : formats::JumpLinkReg
{
	using formats::JumpLinkReg::JumpLinkReg;
};

struct JALI : formats::JumpLinkI28
{
	using formats::JumpLinkI28::JumpLinkI28;
};

struct CJI : formats::JumpI12
{
	using formats::JumpI12::JumpI12;
};

struct BSEXT8 : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BSEXT16 : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BZEXT8 : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BZEXT16 : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct INEG : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct ISUB : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct IADD : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct IADDSI : formats::ALURegS4
{
	using formats::ALURegS4::ALURegS4;
};

struct IADDSIW : formats::ALUWideAdd
{
	using formats::ALUWideAdd::ALUWideAdd;
};

struct IADDSITNZ : formats::ALURegS4
{
	using formats::ALURegS4::ALURegS4;
};

struct BAND : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BOR : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BXOR : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BSL : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BSR : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BASR : formats::ALURegReg
{
	using formats::ALURegReg::ALURegReg;
};

struct BSLI : formats::ALURegS5
{
	using formats::ALURegS5::ALURegS5;
};

struct BSRITLSB : formats::ALURegS5
{
	using formats::ALURegS5::ALURegS5;
};

struct BASRI : formats::ALURegS5
{
	using formats::ALURegS5::ALURegS5;
};

struct INTOFF : formats::NoArg
{
	using formats::NoArg::NoArg;
};

struct INTON : formats::NoArg
{
	using formats::NoArg::NoArg;
};

struct INTRET : formats::NoArg
{
	using formats::NoArg::NoArg;
};

struct INTWAIT : formats::NoArg
{
	using formats::NoArg::NoArg;
};

struct Unknown
{
	Instruction raw;
	static constexpr std::size_t length = 2;
};

using AnyInstruction = std::variant<
	L8,
	L16,
	L32,
	CLR,
	L8OW,
	L16OW,
	L32OW,
	LR,
	LS8,
	LS16,
	LS8OW,
	LS16OW,
	L8O,
	L16O,
	L32O,
	LS8O,
	LS16O,
	LSI,
	LSIH,
	LSIW,
	LIPREL,
	S8,
	S16,
	S32,
	PUSH,
	S8OW,
	S16OW,
	S32OW,
	BRK,
	S8O,
	S16O,
	S32O,
	TLTU,
	TLTS,
	TGEU,
	TGES,
	TE,
	TNE,
	TLTSI,
	TGESI,
	TEI,
	TNEI,
	TBZ,
	PLL32,
	J,
	CJ,
	JAL,
	JALI,
	CJI,
	BSEXT8,
	BSEXT16,
	BZEXT8,
	BZEXT16,
	INEG,
	ISUB,
	IADD,
	IADDSI,
	IADDSIW,
	IADDSITNZ,
	BAND,
	BOR,
	BXOR,
	BSL,
	BSR,
	BASR,
	BSLI,
	BSRITLSB,
	BASRI,
	INTOFF,
	INTON,
	INTRET,
	INTWAIT,
	Unknown
>;

// TODO: move to own file cause lol
inline std::string disassemble(const AnyInstruction& insn)
{
	constexpr static auto rn = register_name;

	return std::visit(overloaded{
		[&](L8 x) { return fmt::format("l8(addr={}, dst={})", rn(x.addr), rn(x.dst)); },
		[&](L16 x) { return fmt::format("l16(addr={}, dst={})", rn(x.addr), rn(x.dst)); },
		[&](L32 x) { return fmt::format("l32(addr={}, dst={})", rn(x.addr), rn(x.dst)); },
		[&](CLR x) { return fmt::format("c_lr(src={}, dst={})", rn(x.src), rn(x.dst)); },
		[&](L8OW x) { return fmt::format("l8ow(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset); },
		[&](L16OW x) { return fmt::format("l16ow(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset << 1); },
		[&](L32OW x) { return fmt::format("l32ow(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset << 2); },
		[&](LR x) { return fmt::format("lr(src={}, dst={})", rn(x.src), rn(x.dst)); },
		[&](auto) { return std::string("<unimplemented>"); },
		[&](LS8 x) { return fmt::format("ls8(addr={}, dst={})", rn(x.addr), rn(x.dst)); },
		[&](LS16 x) { return fmt::format("ls16(addr={}, dst={})", rn(x.addr), rn(x.dst)); },
		[&](LS8OW x) { return fmt::format("ls8ow(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset); },
		[&](LS16OW x) { return fmt::format("ls16ow(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset << 1); },
		[&](L8O x) { return fmt::format("l8o(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset); },
		[&](L16O x) { return fmt::format("l16o(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset << 1); },
		[&](L32O x) { return fmt::format("l32o(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset << 2); },
		[&](LS8O x) { return fmt::format("ls8o(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset); },
		[&](LS16O x) { return fmt::format("ls16o(base={}, dst={}, offset={})", rn(x.base_addr), rn(x.dst), x.offset << 1); },
		[&](LSI x) { return fmt::format("lsi(dst={}, imm={})", rn(x.dst), x.imm); },
		[&](LSIH x) { return fmt::format("lsih(dst={}, imm={})", rn(x.dst), x.imm); },
		[&](LSIW x) { return fmt::format("lsiw(dst={}, imm={})", rn(x.dst), x.imm); },
		[&](LIPREL x) { return fmt::format("liprel(dst={}, imm={})", rn(x.dst), x.imm << 1); },
		[&](S8 x) { return fmt::format("s8(addr={}, src={})", rn(x.addr), rn(x.src)); },
		[&](S16 x) { return fmt::format("s16(addr={}, src={})", rn(x.addr), rn(x.src)); },
		[&](S32 x) { return fmt::format("s32(addr={}, src={})", rn(x.addr), rn(x.src)); },
		[&](PUSH x) { return fmt::format("s32(src={})", rn(x.src)); },
		[&](S8OW x) { return fmt::format("s8ow(base={}, src={}, offset={})", rn(x.base_addr), rn(x.src), x.offset); },
		[&](S16OW x) { return fmt::format("s16ow(base={}, src={}, offset={})", rn(x.base_addr), rn(x.src), x.offset << 1); },
		[&](S32OW x) { return fmt::format("s32ow(base={}, src={}, offset={})", rn(x.base_addr), rn(x.src), x.offset << 2); },
		[&](BRK x) { return fmt::format("brk()"); },
		[&](S8O x) { return fmt::format("s8o(base={}, src={}, offset={})", rn(x.base_addr), rn(x.src), x.offset); },
		[&](S16O x) { return fmt::format("s16o(base={}, src={}, offset={})", rn(x.base_addr), rn(x.src), x.offset << 1); },
		[&](S32O x) { return fmt::format("s32o(base={}, src={}, offset={})", rn(x.base_addr), rn(x.src), x.offset << 2); },
		[&](TLTU x) { return fmt::format("tltu(a={}, b={})", rn(x.a), rn(x.b)); },
		[&](TLTS x) { return fmt::format("tlts(a={}, b={})", rn(x.a), rn(x.b)); },
		[&](TGEU x) { return fmt::format("tgeu(a={}, b={})", rn(x.a), rn(x.b)); },
		[&](TGES x) { return fmt::format("tges(a={}, b={})", rn(x.a), rn(x.b)); },
		[&](TE x) { return fmt::format("te(a={}, b={})", rn(x.a), rn(x.b)); },
		[&](TNE x) { return fmt::format("tne(a={}, b={})", rn(x.a), rn(x.b)); },
		[&](TLTSI x) { return fmt::format("tltsi(a={}, b={})", rn(x.a), x.b); },
		[&](TGESI x) { return fmt::format("tgesi(a={}, b={})", rn(x.a), x.b); },
		[&](TEI x) { return fmt::format("tei(a={}, b={})", rn(x.a), x.b); },
		[&](TNEI x) { return fmt::format("tnei(a={}, b={})", rn(x.a), x.b); },
		[&](TBZ x) { return fmt::format("tltsi(a={})", rn(x.a)); },
		[&](PLL32 x) { return fmt::format("pl_l32(dst={}, offset={})", rn(x.dst), x.offset); },
		[&](J x) { return fmt::format("j(target={})", rn(x.target)); },
		[&](CJ x) { return fmt::format("c_j(target={})", rn(x.target)); },
		[&](JAL x) { return fmt::format("jal(target={}, dst={})", rn(x.target), rn(x.dst)); },
		[&](JALI x) { return fmt::format("jali(target={})  # dst = rret", x.relative_target << 1); },
		[&](CJI x) { return fmt::format("c_ji(target={})", x.relative_target << 1); },
		[&](BSEXT8 x) { return fmt::format("bsext8(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BSEXT16 x) { return fmt::format("bsext16(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BZEXT8 x) { return fmt::format("bzext8(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BZEXT16 x) { return fmt::format("bzext16(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](INEG x) { return fmt::format("ineg(dst={}, a={})", rn(x.a_dst), rn(x.b)); },
		[&](ISUB x) { return fmt::format("idst(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](IADD x) { return fmt::format("iadd(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](IADDSI x) { return fmt::format("iaddsi(a_dst={}, b={})", rn(x.a_dst), x.b); },
		[&](IADDSIW x) { return fmt::format("iaddsiw(dst={}, a={}, b={})", rn(x.dst), rn(x.a), x.b); },
		[&](IADDSITNZ x) { return fmt::format("iaddsi_tnz(a_dst={}, b={})", rn(x.a_dst), x.b); },
		[&](BAND x) { return fmt::format("band(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BOR x) { return fmt::format("bor(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BXOR x) { return fmt::format("bxor(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BSL x) { return fmt::format("bsl(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BSR x) { return fmt::format("bsr(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BASR x) { return fmt::format("basr(a_dst={}, b={})", rn(x.a_dst), rn(x.b)); },
		[&](BSLI x) { return fmt::format("bsli(a_dst={}, b={})", rn(x.a_dst), x.b); },
		[&](BSRITLSB x) { return fmt::format("bsri_tlsb(a_dst={}, b={})", rn(x.a_dst), x.b); },
		[&](BASRI x) { return fmt::format("basri(a_dst={}, b={})", rn(x.a_dst), x.b); },
		[&](INTOFF x) { return fmt::format("intoff()"); },
		[&](INTON x) { return fmt::format("inton()"); },
		[&](INTRET x) { return fmt::format("intret()"); },
		[&](INTWAIT x) { return fmt::format("intwait()"); },
		[&](Unknown x) { return fmt::format("uint32_t({:#010x})", x.raw); },

	}, insn);
}

inline AnyInstruction decode(u32 insn)
{
	const u32 o8 = (insn >> 8) & 0b1111'1111;
	const u32 o7 = o8 & 0b1111'1110;
	const u32 o6 = o8 & 0b1111'1100;
	const u32 o4 = o8 & 0b1111'0000;

	if (o8 == 0b0000'0000) { return L8(insn); }
	if (o8 == 0b0000'0001) { return L16(insn); }
	if (o8 == 0b0000'0010) { return L32(insn); }
	if (o8 == 0b0000'0011) { return CLR(insn); }
	if (o8 == 0b0000'0100) { return L8OW(insn); }
	if (o8 == 0b0000'0101) { return L16OW(insn); }
	if (o8 == 0b0000'0110) { return L32OW(insn); }
	if (o8 == 0b0000'0111) { return LR(insn); }
	if (o8 == 0b0000'1000) { return LS8(insn); }
	if (o8 == 0b0000'1001) { return LS16(insn); }
	if (o8 == 0b0000'1010) { return LS8OW(insn); }
	if (o8 == 0b0000'1011) { return LS16OW(insn); }
	if (o6 == 0b0000'1100) { return L8O(insn); }
	if (o6 == 0b0001'0000) { return L16O(insn); }
	if (o6 == 0b0001'0100) { return L32O(insn); }
	if (o6 == 0b0001'1000) { return LS8O(insn); }
	if (o6 == 0b0001'1100) { return LS16O(insn); }
	if (o4 == 0b0010'0000) { return LSI(insn); }
	if (o4 == 0b0011'0000) { return LSIH(insn); }
	if (o4 == 0b0100'0000) { return LSIW(insn); }
	if (o4 == 0b0101'0000) { return LIPREL(insn); }
	if (o8 == 0b0110'0000) { return S8(insn); }
	if (o8 == 0b0110'0001) { return S16(insn); }
	if (o8 == 0b0110'0010) { return S32(insn); }
	if (o8 == 0b0110'0011) { return PUSH(insn); }
	if (o8 == 0b0110'0100) { return S8OW(insn); }
	if (o8 == 0b0110'0101) { return S16OW(insn); }
	if (o8 == 0b0110'0110) { return S32OW(insn); }
	if (o8 == 0b0110'0111) { return BRK(insn); }
	if (o6 == 0b0110'1000) { return S8O(insn); }
	if (o6 == 0b0110'1100) { return S16O(insn); }
	if (o6 == 0b0111'0000) { return S32O(insn); }
	if (o8 == 0b0111'0100) { return TLTU(insn); }
	if (o8 == 0b0111'0101) { return TLTS(insn); }
	if (o8 == 0b0111'0110) { return TGEU(insn); }
	if (o8 == 0b0111'0111) { return TGES(insn); }
	if (o8 == 0b0111'1000) { return TE(insn); }
	if (o8 == 0b0111'1001) { return TNE(insn); }
	if (o8 == 0b0111'1010) { return TLTSI(insn); }
	if (o8 == 0b0111'1011) { return TGESI(insn); }
	if (o8 == 0b0111'1100) { return TEI(insn); }
	if (o8 == 0b0111'1101) { return TNEI(insn); }
	if (o8 == 0b0111'1110) { return TBZ(insn); }
	if (o4 == 0b1000'0000) { return PLL32(insn); }
	if (o8 == 0b1001'0000) { return J(insn); }
	if (o8 == 0b1001'0001) { return CJ(insn); }
	if (o8 == 0b1001'0010) { return JAL(insn); }
	if (o4 == 0b1010'0000) { return JALI(insn); }
	if (o4 == 0b1011'0000) { return CJI(insn); }
	if (o8 == 0b1100'0000) { return BSEXT8(insn); }
	if (o8 == 0b1100'0001) { return BSEXT16(insn); }
	if (o8 == 0b1100'0010) { return BZEXT8(insn); }
	if (o8 == 0b1100'0011) { return BZEXT16(insn); }
	if (o8 == 0b1100'0100) { return INEG(insn); }
	if (o8 == 0b1100'0101) { return ISUB(insn); }
	if (o8 == 0b1100'0110) { return IADD(insn); }
	if (o8 == 0b1100'0111) { return IADDSI(insn); }
	if (o8 == 0b1100'1000) { return IADDSIW(insn); }
	if (o8 == 0b1100'1001) { return IADDSITNZ(insn); }
	if (o8 == 0b1100'1010) { return BAND(insn); }
	if (o8 == 0b1100'1011) { return BOR(insn); }
	if (o8 == 0b1100'1100) { return BXOR(insn); }
	if (o8 == 0b1100'1101) { return BSL(insn); }
	if (o8 == 0b1100'1110) { return BSR(insn); }
	if (o8 == 0b1100'1111) { return BASR(insn); }
	if (o7 == 0b1101'0000) { return BSLI(insn); }
	if (o7 == 0b1101'0010) { return BSRITLSB(insn); }
	if (o7 == 0b1101'0100) { return BASRI(insn); }
	if (o8 == 0b1110'0000) { return INTOFF(insn); }
	if (o8 == 0b1110'0001) { return INTON(insn); }
	if (o8 == 0b1110'0010) { return INTRET(insn); }
	if (o8 == 0b1110'0011) { return INTWAIT(insn); }
	return Unknown();
}

}