#pragma once

constexpr auto is_space(char c) -> bool { return c == ' ' || c == '\t'; }
constexpr auto is_alpha(char c) -> bool { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr auto is_num(char c) -> bool { return c >= '0' && c <= '9'; }
constexpr auto is_newline(char c) -> bool { return c == '\n'; }

constexpr auto is_identifier_begin(char c) -> bool { return is_alpha(c) || c == '_'; }
constexpr auto is_identifier(char c) -> bool { return is_identifier_begin(c) || is_num(c); }
