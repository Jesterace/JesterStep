#pragma once

#include <windows.h>
#include <string>
#include <vector>

inline const wchar_t* APP_CLASS = L"JesterStepPanel";
inline const wchar_t* SETTINGS_CLASS = L"JesterStepSettings";
inline const wchar_t* LAUNCHER_EDITOR_CLASS = L"JesterStepLauncherEditor";

inline const UINT APPBAR_CALLBACK = WM_APP + 100;

inline const int IDC_PANEL_POSITION = 2001;
inline const int IDC_PANEL_HEIGHT = 2002;
inline const int IDC_APPLY = 2003;
inline const int IDC_SAVE = 2004;
inline const int IDC_CANCEL = 2005;
inline const int IDC_BG_COLOR = 2006;
inline const int IDC_BG_PICK = 2007;
inline const int IDC_TEXT_COLOR = 2008;
inline const int IDC_TEXT_PICK = 2009;
inline const int IDC_LAUNCHERS_BUTTON = 2010;
inline const int IDC_ACCENT_COLOR = 2011;
inline const int IDC_ACCENT_PICK = 2012;

inline const int IDC_LAUNCHER_LIST = 4001;
inline const int IDC_LAUNCHER_NAME = 4002;
inline const int IDC_LAUNCHER_COMMAND = 4003;
inline const int IDC_LAUNCHER_ADD = 4004;
inline const int IDC_LAUNCHER_UPDATE = 4005;
inline const int IDC_LAUNCHER_REMOVE = 4006;
inline const int IDC_LAUNCHER_UP = 4007;
inline const int IDC_LAUNCHER_DOWN = 4008;
inline const int IDC_LAUNCHER_SAVE = 4009;
inline const int IDC_LAUNCHER_CLOSE = 4010;

inline const int MENU_SETTINGS = 1005;
inline const int MENU_RELOAD = 1003;
inline const int MENU_EXIT = 1004;

inline const int MENU_LAUNCHER_BASE = 3000;
inline const int MENU_LAUNCHER_LIMIT = 100;

struct Launcher {
    std::wstring name;
    std::wstring command;
};

struct TaskEntry {
    HWND hwnd = nullptr;
    std::wstring title;
};

struct AppState {
    HWND panel_hwnd = nullptr;
    HWND settings_hwnd = nullptr;
    HWND launcher_editor_hwnd = nullptr;

    bool appbar_registered = false;
    bool panel_top = true;
    int panel_height = 36;

    COLORREF panel_bg_color = RGB(24, 24, 28);
    COLORREF panel_text_color = RGB(230, 230, 230);
    COLORREF accent_color = RGB(64, 156, 255);

    std::vector<Launcher> launchers;
    std::vector<TaskEntry> tasks;
    HWND foreground_hwnd = nullptr;
    int hovered_task_index = -1;
};

inline AppState* app_state_from_hwnd(HWND hwnd) {
    return reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

inline HMENU control_id(int id) {
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

inline int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}
