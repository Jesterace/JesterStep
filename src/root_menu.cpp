#include "root_menu.h"

#include "appbar.h"
#include "settings_store.h"
#include "settings_window.h"
#include "shared_state.h"

#include <shellapi.h>

static void launch_app(const wchar_t* app) {
    ShellExecuteW(nullptr, L"open", app, nullptr, nullptr, SW_SHOWNORMAL);
}

static void reload_saved_settings() {
    load_settings();

    if (g_panel_hwnd) {
        position_appbar(g_panel_hwnd);
        InvalidateRect(g_panel_hwnd, nullptr, TRUE);
    }

    if (g_settings_hwnd) {
        refresh_settings_window_controls(g_settings_hwnd);
    }
}
void show_root_menu(HWND hwnd, int x, int y) {
    HMENU menu = CreatePopupMenu();

    for (size_t i = 0; i < g_launchers.size() && i < MENU_LAUNCHER_LIMIT; ++i) {
        AppendMenuW(
            menu,
            MF_STRING,
            MENU_LAUNCHER_BASE + static_cast<UINT>(i),
            g_launchers[i].name.c_str()
        );
    }

    if (!g_launchers.empty()) {
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
        cmd < MENU_LAUNCHER_BASE + static_cast<int>(g_launchers.size())
    ) {
        int index = cmd - MENU_LAUNCHER_BASE;
        launch_app(g_launchers[index].command.c_str());
        return;
    }

    switch (cmd) {
        case MENU_RELOAD:
            reload_saved_settings();
            break;

        case MENU_EXIT:
            DestroyWindow(hwnd);
            break;

        case MENU_SETTINGS:
            show_settings_window(hwnd);
            break;

        default:
            break;
    }
}
