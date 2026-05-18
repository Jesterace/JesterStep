#include "panel_window.h"
#include "settings_store.h"
#include "shared_state.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    load_settings();

    g_panel_hwnd = create_panel_window(instance);

    if (!g_panel_hwnd) {
        return 1;
    }

    ShowWindow(g_panel_hwnd, SW_SHOW);
    UpdateWindow(g_panel_hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}