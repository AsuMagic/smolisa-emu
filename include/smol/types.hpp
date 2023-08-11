#pragma once

#include <cstddef>
#include <cstdint>

using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;
using s32 = std::int32_t;
using s16 = std::int16_t;
using s8 = std::int8_t;

using Addr = u32;
using Word = u32;

/// Type which can hold a full instruction, including extended formats.
/// Typical instructions are 16-bit.
/// Instructions are encoded in little-endian format at byte level.
using Instruction = u32;