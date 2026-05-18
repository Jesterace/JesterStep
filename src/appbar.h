#pragma once

#include <windows.h>

struct AppState;

void position_appbar(AppState& state, HWND hwnd);
bool register_appbar(AppState& state, HWND hwnd);
void unregister_appbar(AppState& state, HWND hwnd);
void notify_appbar_windowpos(AppState& state, HWND hwnd);
