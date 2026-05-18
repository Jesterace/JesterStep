#pragma once

#include <string>

struct AppState;

std::wstring get_settings_path();
void load_settings(AppState& state);
bool save_settings(const AppState& state);
