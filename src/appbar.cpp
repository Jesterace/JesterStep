#include "appbar.h"

#include "shared_state.h"

#include <shellapi.h>

void position_appbar(AppState& state, HWND hwnd) {
    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;
    abd.uEdge = state.panel_top ? ABE_TOP : ABE_BOTTOM;

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);

    abd.rc.left = 0;
    abd.rc.right = screen_w;

    if (state.panel_top) {
        abd.rc.top = 0;
        abd.rc.bottom = state.panel_height;
    } else {
        abd.rc.top = screen_h - state.panel_height;
        abd.rc.bottom = screen_h;
    }

    SHAppBarMessage(ABM_QUERYPOS, &abd);

    if (state.panel_top) {
        abd.rc.bottom = abd.rc.top + state.panel_height;
    } else {
        abd.rc.top = abd.rc.bottom - state.panel_height;
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

bool register_appbar(AppState& state, HWND hwnd) {
    if (state.appbar_registered) {
        position_appbar(state, hwnd);
        return true;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;
    abd.uCallbackMessage = APPBAR_CALLBACK;

    if (!SHAppBarMessage(ABM_NEW, &abd)) {
        return false;
    }

    state.appbar_registered = true;
    position_appbar(state, hwnd);
    return true;
}

void unregister_appbar(AppState& state, HWND hwnd) {
    if (!state.appbar_registered) {
        return;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;

    SHAppBarMessage(ABM_REMOVE, &abd);
    state.appbar_registered = false;
}

void notify_appbar_windowpos(AppState& state, HWND hwnd) {
    if (!state.appbar_registered) {
        return;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;

    SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
}
