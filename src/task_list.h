#pragma once

#include <windows.h>

struct AppState;

void refresh_task_list(AppState& state);
void activate_task_window(HWND hwnd);
