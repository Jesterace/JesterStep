#include "appbar.h"

#include "shared_state.h"

#include <shellapi.h>

void position_appbar(HWND hwnd) {
    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;
    abd.uEdge = g_panel_top ? ABE_TOP : ABE_BOTTOM;

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);

    abd.rc.left = 0;
    abd.rc.right = screen_w;

    if (g_panel_top) {
        abd.rc.top = 0;
        abd.rc.bottom = g_panel_height;
    } else {
        abd.rc.top = screen_h - g_panel_height;
        abd.rc.bottom = screen_h;
    }

    SHAppBarMessage(ABM_QUERYPOS, &abd);

    if (g_panel_top) {
        abd.rc.bottom = abd.rc.top + g_panel_height;
    } else {
        abd.rc.top = abd.rc.bottom - g_panel_height;
    }

    SHAppBarMessage(ABM_SETPOS, &abd);

    MoveWindow(
        hwnd,
        abd.rc.left,
        abd.rc.top,
        abd.rc.right - abd.rc.left,
        abd.rc.bottom - abd.rc.top,
        TRUE
    );
}

bool register_appbar(HWND hwnd) {
    if (g_appbar_registered) {
        position_appbar(hwnd);
        return true;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;
    abd.uCallbackMessage = APPBAR_CALLBACK;

    if (!SHAppBarMessage(ABM_NEW, &abd)) {
        return false;
    }

    g_appbar_registered = true;
    position_appbar(hwnd);
    return true;
}

void unregister_appbar(HWND hwnd) {
    if (!g_appbar_registered) {
        return;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;

    SHAppBarMessage(ABM_REMOVE, &abd);
    g_appbar_registered = false;
}

void notify_appbar_windowpos(HWND hwnd) {
    if (!g_appbar_registered) {
        return;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;

    SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
}
