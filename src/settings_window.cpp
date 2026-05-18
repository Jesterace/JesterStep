#include "settings_window.h"

#include "appbar.h"
#include "color_utils.h"
#include "launcher_editor_window.h"
#include "settings_store.h"
#include "shared_state.h"

#include <commdlg.h>
#include <cstdlib>
#include <string>

void refresh_settings_window_controls(HWND hwnd) {
    HWND position_combo = GetDlgItem(hwnd, IDC_PANEL_POSITION);
    HWND height_edit = GetDlgItem(hwnd, IDC_PANEL_HEIGHT);
    HWND bg_edit = GetDlgItem(hwnd, IDC_BG_COLOR);
    HWND text_edit = GetDlgItem(hwnd, IDC_TEXT_COLOR);

    if (position_combo) {
        SendMessageW(position_combo, CB_SETCURSEL, g_panel_top ? 0 : 1, 0);
    }

    if (height_edit) {
        wchar_t height_text[32]{};
        swprintf_s(height_text, L"%d", g_panel_height);
        SetWindowTextW(height_edit, height_text);
    }

    if (bg_edit) {
        std::wstring bg = color_to_hex(g_panel_bg_color);
        SetWindowTextW(bg_edit, bg.c_str());
    }

    if (text_edit) {
        std::wstring text = color_to_hex(g_panel_text_color);
        SetWindowTextW(text_edit, text.c_str());
    }
}

static bool apply_settings_from_window(HWND hwnd) {
    HWND position_combo = GetDlgItem(hwnd, IDC_PANEL_POSITION);
    HWND height_edit = GetDlgItem(hwnd, IDC_PANEL_HEIGHT);
    HWND bg_edit = GetDlgItem(hwnd, IDC_BG_COLOR);
    HWND text_edit = GetDlgItem(hwnd, IDC_TEXT_COLOR);

    LRESULT selected = SendMessageW(position_combo, CB_GETCURSEL, 0, 0);
    bool new_panel_top = selected != 1;

    wchar_t height_text[32]{};
    GetWindowTextW(height_edit, height_text, 32);

    int new_height = clamp_int(_wtoi(height_text), 24, 96);

    wchar_t bg_text[32]{};
    GetWindowTextW(bg_edit, bg_text, 32);

    COLORREF new_bg{};
    if (!parse_hex_color(bg_text, &new_bg)) {
        MessageBoxW(
            hwnd,
            L"Background color must be a hex color like #18181C.",
            L"JesterStep",
            MB_OK | MB_ICONERROR
        );
        return false;
    }

    wchar_t text_color_text[32]{};
    GetWindowTextW(text_edit, text_color_text, 32);

    COLORREF new_text{};
    if (!parse_hex_color(text_color_text, &new_text)) {
        MessageBoxW(
            hwnd,
            L"Text color must be a hex color like #E6E6E6.",
            L"JesterStep",
            MB_OK | MB_ICONERROR
        );
        return false;
    }

    g_panel_top = new_panel_top;
    g_panel_height = new_height;
    g_panel_bg_color = new_bg;
    g_panel_text_color = new_text;

    refresh_settings_window_controls(hwnd);

    if (g_panel_hwnd) {
        position_appbar(g_panel_hwnd);
        InvalidateRect(g_panel_hwnd, nullptr, TRUE);
    }

    return true;
}

static void choose_color_for_edit(HWND hwnd, int edit_id, COLORREF current_color) {
    static COLORREF custom_colors[16]{};

    CHOOSECOLORW cc{};
    cc.lStructSize = sizeof(CHOOSECOLORW);
    cc.hwndOwner = hwnd;
    cc.rgbResult = current_color;
    cc.lpCustColors = custom_colors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorW(&cc)) {
        std::wstring hex = color_to_hex(cc.rgbResult);
        SetWindowTextW(GetDlgItem(hwnd, edit_id), hex.c_str());
    }
}

static LRESULT CALLBACK settings_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindowExW(
                0,
                L"STATIC",
                L"Panel position:",
                WS_CHILD | WS_VISIBLE,
                16,
                24,
                110,
                24,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr
            );

            HWND position_combo = CreateWindowExW(
                0,
                L"COMBOBOX",
                nullptr,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                140,
                20,
                170,
                120,
                hwnd,
                control_id(IDC_PANEL_POSITION),
                GetModuleHandleW(nullptr),
                nullptr
            );

            SendMessageW(position_combo, CB_ADDSTRING, 0, (LPARAM)L"Top");
            SendMessageW(position_combo, CB_ADDSTRING, 0, (LPARAM)L"Bottom");
            SendMessageW(position_combo, CB_SETCURSEL, g_panel_top ? 0 : 1, 0);

            CreateWindowExW(
                0,
                L"STATIC",
                L"Panel height:",
                WS_CHILD | WS_VISIBLE,
                16,
                64,
                110,
                24,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr
            );

            wchar_t height_text[32]{};
            swprintf_s(height_text, L"%d", g_panel_height);

            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                height_text,
                WS_CHILD | WS_VISIBLE | ES_NUMBER,
                140,
                60,
                80,
                24,
                hwnd,
                control_id(IDC_PANEL_HEIGHT),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"STATIC",
                L"Height range: 24 to 96 px",
                WS_CHILD | WS_VISIBLE,
                140,
                88,
                180,
                20,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"STATIC",
                L"Background:",
                WS_CHILD | WS_VISIBLE,
                16,
                124,
                110,
                24,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr
            );

            std::wstring bg = color_to_hex(g_panel_bg_color);

            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                bg.c_str(),
                WS_CHILD | WS_VISIBLE,
                140,
                120,
                100,
                24,
                hwnd,
                control_id(IDC_BG_COLOR),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Pick...",
                WS_CHILD | WS_VISIBLE,
                252,
                119,
                80,
                26,
                hwnd,
                control_id(IDC_BG_PICK),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"STATIC",
                L"Text:",
                WS_CHILD | WS_VISIBLE,
                16,
                164,
                110,
                24,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr
            );

            std::wstring text = color_to_hex(g_panel_text_color);

            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                text.c_str(),
                WS_CHILD | WS_VISIBLE,
                140,
                160,
                100,
                24,
                hwnd,
                control_id(IDC_TEXT_COLOR),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Pick...",
                WS_CHILD | WS_VISIBLE,
                252,
                159,
                80,
                26,
                hwnd,
                control_id(IDC_TEXT_PICK),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Launchers...",
                WS_CHILD | WS_VISIBLE,
                140,
                204,
                120,
                28,
                hwnd,
                control_id(IDC_LAUNCHERS_BUTTON),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Apply",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                44,
                260,
                80,
                28,
                hwnd,
                control_id(IDC_APPLY),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Save",
                WS_CHILD | WS_VISIBLE,
                156,
                260,
                80,
                28,
                hwnd,
                control_id(IDC_SAVE),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Cancel",
                WS_CHILD | WS_VISIBLE,
                268,
                260,
                80,
                28,
                hwnd,
                control_id(IDC_CANCEL),
                GetModuleHandleW(nullptr),
                nullptr
            );

            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);

            if (id == IDC_APPLY) {
                apply_settings_from_window(hwnd);
                return 0;
            }

            if (id == IDC_SAVE) {
                if (!apply_settings_from_window(hwnd)) {
                    return 0;
                }

                if (save_settings()) {
                    std::wstring msg =
                        L"Settings saved to:\n\n" + get_settings_path();

                    MessageBoxW(
                        hwnd,
                        msg.c_str(),
                        L"JesterStep",
                        MB_OK | MB_ICONINFORMATION
                    );
                } else {
                    MessageBoxW(
                        hwnd,
                        L"Could not save settings.",
                        L"JesterStep",
                        MB_OK | MB_ICONERROR
                    );
                }

                return 0;
            }

            if (id == IDC_CANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }

            if (id == IDC_BG_PICK) {
                choose_color_for_edit(hwnd, IDC_BG_COLOR, g_panel_bg_color);
                return 0;
            }

            if (id == IDC_TEXT_PICK) {
                choose_color_for_edit(hwnd, IDC_TEXT_COLOR, g_panel_text_color);
                return 0;
            }

            if (id == IDC_LAUNCHERS_BUTTON) {
                show_launcher_editor(hwnd);
                return 0;
            }

            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_NCDESTROY:
            g_settings_hwnd = nullptr;
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void show_settings_window(HWND parent) {
    if (g_settings_hwnd) {
        ShowWindow(g_settings_hwnd, SW_SHOW);
        SetForegroundWindow(g_settings_hwnd);
        return;
    }

    WNDCLASSW wc{};
    wc.lpfnWndProc = settings_wndproc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = SETTINGS_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    g_settings_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        SETTINGS_CLASS,
        L"JesterStep Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        420,
        360,
        parent,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (g_settings_hwnd) {
        ShowWindow(g_settings_hwnd, SW_SHOW);
        UpdateWindow(g_settings_hwnd);
    }
}
