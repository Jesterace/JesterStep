#pragma once

#include <windows.h>

void position_appbar(HWND hwnd);
bool register_appbar(HWND hwnd);
void unregister_appbar(HWND hwnd);
void notify_appbar_windowpos(HWND hwnd);