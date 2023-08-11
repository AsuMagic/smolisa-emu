#pragma once

namespace masks
{
constexpr auto lower_nibble = 0b0000'1111;
constexpr auto upper_nibble = 0b1111'0000;
constexpr auto upper_byte   = 0b1111'1111'0000'0000;
constexpr auto lower_byte   = 0b0000'0000'1111'1111;

} // namespace masks
