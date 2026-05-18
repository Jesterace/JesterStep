#include "task_list.h"

#include "shared_state.h"

#include <string>

static bool is_jesterstep_window(AppState& state, HWND hwnd) {
    if (
        hwnd == state.panel_hwnd ||
        hwnd == state.settings_hwnd ||
        hwnd == state.launcher_editor_hwnd
    ) {
        return true;
    }

    wchar_t class_name[128]{};
    GetClassNameW(hwnd, class_name, 128);

    return (
        lstrcmpW(class_name, APP_CLASS) == 0 ||
        lstrcmpW(class_name, SETTINGS_CLASS) == 0 ||
        lstrcmpW(class_name, LAUNCHER_EDITOR_CLASS) == 0
    );
}

static BOOL CALLBACK enum_task_window(HWND hwnd, LPARAM lparam) {
    AppState* state = reinterpret_cast<AppState*>(lparam);

    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }

    if (is_jesterstep_window(*state, hwnd)) {
        return TRUE;
    }

    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((ex_style & WS_EX_TOOLWINDOW) != 0) {
        return TRUE;
    }

    if (GetWindow(hwnd, GW_OWNER) && (ex_style & WS_EX_APPWINDOW) == 0) {
        return TRUE;
    }

    int title_length = GetWindowTextLengthW(hwnd);
    if (title_length <= 0) {
        return TRUE;
    }

    std::wstring title(static_cast<size_t>(title_length) + 1, L'\0');
    GetWindowTextW(hwnd, title.data(), title_length + 1);
    title.resize(static_cast<size_t>(title_length));

    if (title.empty()) {
        return TRUE;
    }

    state->tasks.push_back({hwnd, title});
    return TRUE;
}

void refresh_task_list(AppState& state) {
    state.tasks.clear();
    state.foreground_hwnd = GetForegroundWindow();
    EnumWindows(enum_task_window, reinterpret_cast<LPARAM>(&state));

    if (state.hovered_task_index >= static_cast<int>(state.tasks.size())) {
        state.hovered_task_index = -1;
    }
}

void activate_task_window(HWND hwnd) {
    if (!IsWindow(hwnd)) {
        return;
    }

    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }

    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
}
