#pragma once

constexpr bool is_space(char c) { return c == ' ' || c == '\t'; }
constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr bool is_num(char c) { return c >= '0' && c <= '9'; }
constexpr bool is_newline(char c) { return c == '\n'; }

constexpr bool is_identifier_begin(char c) { return is_alpha(c) || c == '_'; }
constexpr bool is_identifier(char c) { return is_identifier_begin(c) || is_num(c); }
