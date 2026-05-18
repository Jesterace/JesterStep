#pragma once

#include <windows.h>
#include <string>

bool parse_hex_color(const std::wstring& text, COLORREF* out_color);
std::wstring color_to_hex(COLORREF color);