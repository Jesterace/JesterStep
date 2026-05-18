#pragma once

#include <windows.h>

struct AppState;

HWND create_panel_window(HINSTANCE instance, AppState& state);
