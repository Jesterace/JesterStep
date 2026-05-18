#pragma once

#include <windows.h>

struct AppState;

void refresh_settings_window_controls(AppState& state, HWND hwnd);
void show_settings_window(AppState& state, HWND parent);
