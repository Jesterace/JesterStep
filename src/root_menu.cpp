#include "root_menu.h"

#include "appbar.h"
#include "settings_store.h"
#include "settings_window.h"
#include "shared_state.h"
#include "task_list.h"

#include <shellapi.h>

static void launch_app(const wchar_t* app) {
    ShellExecuteW(nullptr, L"open", app, nullptr, nullptr, SW_SHOWNORMAL);
}

static void reload_saved_settings(AppState& state) {
    load_settings(state);

    if (state.panel_hwnd) {
        position_appbar(state, state.panel_hwnd);
        refresh_task_list(state);
        InvalidateRect(state.panel_hwnd, nullptr, TRUE);
    }

    if (state.settings_hwnd) {
        refresh_settings_window_controls(state, state.settings_hwnd);
    }
}

void show_root_menu(AppState& state, HWND hwnd, int x, int y) {
    HMENU menu = CreatePopupMenu();

    for (size_t i = 0; i < state.launchers.size() && i < MENU_LAUNCHER_LIMIT; ++i) {
        AppendMenuW(
            menu,
            MF_STRING,
            MENU_LAUNCHER_BASE + static_cast<UINT>(i),
            state.launchers[i].name.c_str()
        );
    }

    if (!state.launchers.empty()) {
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }

    AppendMenuW(menu, MF_STRING, MENU_SETTINGS, L"Settings...");
    AppendMenuW(menu, MF_STRING, MENU_RELOAD, L"Reload");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MENU_EXIT, L"Exit JesterStep");

    SetForegroundWindow(hwnd);

    int cmd = TrackPopupMenu(
        menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON,
        x,
        y,
        0,
        hwnd,
        nullptr
    );

    DestroyMenu(menu);

    if (
        cmd >= MENU_LAUNCHER_BASE &&
        cmd < MENU_LAUNCHER_BASE + static_cast<int>(state.launchers.size())
    ) {
        int index = cmd - MENU_LAUNCHER_BASE;
        launch_app(state.launchers[index].command.c_str());
        return;
    }

    switch (cmd) {
        case MENU_RELOAD:
            reload_saved_settings(state);
            break;

        case MENU_EXIT:
            DestroyWindow(hwnd);
            break;

        case MENU_SETTINGS:
            show_settings_window(state, hwnd);
            break;

        default:
            break;
    }
}
