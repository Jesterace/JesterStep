#include "panel_window.h"
#include "settings_store.h"
#include "shared_state.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    AppState state;
    load_settings(state);

    state.panel_hwnd = create_panel_window(instance, state);

    if (!state.panel_hwnd) {
        return 1;
    }

    ShowWindow(state.panel_hwnd, SW_SHOW);
    UpdateWindow(state.panel_hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
