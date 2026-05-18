#pragma once

#include <string>

std::wstring get_settings_path();
void load_settings();
bool save_settings();