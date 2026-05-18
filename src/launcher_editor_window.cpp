#include "launcher_editor_window.h"

#include "settings_store.h"
#include "shared_state.h"

#include <windows.h>
#include <string>

static std::wstring format_launcher_display(const Launcher& launcher) {
    return launcher.name + L"  —  " + launcher.command;
}

static int get_selected_launcher_index(HWND hwnd) {
    HWND list = GetDlgItem(hwnd, IDC_LAUNCHER_LIST);
    LRESULT selected = SendMessageW(list, LB_GETCURSEL, 0, 0);

    if (selected == LB_ERR) {
        return -1;
    }

    return static_cast<int>(selected);
}

static void set_launcher_editor_fields(HWND hwnd, int index) {
    HWND name_edit = GetDlgItem(hwnd, IDC_LAUNCHER_NAME);
    HWND command_edit = GetDlgItem(hwnd, IDC_LAUNCHER_COMMAND);

    if (index < 0 || index >= static_cast<int>(g_launchers.size())) {
        SetWindowTextW(name_edit, L"");
        SetWindowTextW(command_edit, L"");
        return;
    }

    SetWindowTextW(name_edit, g_launchers[index].name.c_str());
    SetWindowTextW(command_edit, g_launchers[index].command.c_str());
}

static void refresh_launcher_list(HWND hwnd, int selected_index = -1) {
    HWND list = GetDlgItem(hwnd, IDC_LAUNCHER_LIST);

    SendMessageW(list, LB_RESETCONTENT, 0, 0);

    for (const Launcher& launcher : g_launchers) {
        std::wstring display = format_launcher_display(launcher);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display.c_str()));
    }

    if (!g_launchers.empty()) {
        selected_index = clamp_int(
            selected_index,
            0,
            static_cast<int>(g_launchers.size()) - 1
        );

        SendMessageW(list, LB_SETCURSEL, selected_index, 0);
        set_launcher_editor_fields(hwnd, selected_index);
    } else {
        set_launcher_editor_fields(hwnd, -1);
    }
}

static bool read_launcher_fields(HWND hwnd, Launcher* out_launcher) {
    wchar_t name[256]{};
    wchar_t command[512]{};

    GetWindowTextW(GetDlgItem(hwnd, IDC_LAUNCHER_NAME), name, 256);
    GetWindowTextW(GetDlgItem(hwnd, IDC_LAUNCHER_COMMAND), command, 512);

    if (lstrlenW(name) == 0 || lstrlenW(command) == 0) {
        MessageBoxW(
            hwnd,
            L"Launcher name and command cannot be empty.",
            L"JesterStep",
            MB_OK | MB_ICONERROR
        );
        return false;
    }

    out_launcher->name = name;
    out_launcher->command = command;
    return true;
}

static LRESULT CALLBACK launcher_editor_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"LISTBOX",
                nullptr,
                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
                16,
                16,
                250,
                270,
                hwnd,
                control_id(IDC_LAUNCHER_LIST),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"STATIC",
                L"Name:",
                WS_CHILD | WS_VISIBLE,
                290,
                24,
                80,
                22,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE,
                290,
                48,
                210,
                24,
                hwnd,
                control_id(IDC_LAUNCHER_NAME),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"STATIC",
                L"Command:",
                WS_CHILD | WS_VISIBLE,
                290,
                88,
                100,
                22,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE,
                290,
                112,
                210,
                24,
                hwnd,
                control_id(IDC_LAUNCHER_COMMAND),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Add",
                WS_CHILD | WS_VISIBLE,
                290,
                160,
                95,
                28,
                hwnd,
                control_id(IDC_LAUNCHER_ADD),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Update",
                WS_CHILD | WS_VISIBLE,
                405,
                160,
                95,
                28,
                hwnd,
                control_id(IDC_LAUNCHER_UPDATE),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Remove",
                WS_CHILD | WS_VISIBLE,
                290,
                200,
                95,
                28,
                hwnd,
                control_id(IDC_LAUNCHER_REMOVE),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Move Up",
                WS_CHILD | WS_VISIBLE,
                405,
                200,
                95,
                28,
                hwnd,
                control_id(IDC_LAUNCHER_UP),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Move Down",
                WS_CHILD | WS_VISIBLE,
                290,
                240,
                95,
                28,
                hwnd,
                control_id(IDC_LAUNCHER_DOWN),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Save",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                180,
                315,
                95,
                30,
                hwnd,
                control_id(IDC_LAUNCHER_SAVE),
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Close",
                WS_CHILD | WS_VISIBLE,
                290,
                315,
                95,
                30,
                hwnd,
                control_id(IDC_LAUNCHER_CLOSE),
                GetModuleHandleW(nullptr),
                nullptr
            );

            refresh_launcher_list(hwnd, 0);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);
            int notify = HIWORD(wparam);

            if (id == IDC_LAUNCHER_LIST && notify == LBN_SELCHANGE) {
                int index = get_selected_launcher_index(hwnd);
                set_launcher_editor_fields(hwnd, index);
                return 0;
            }

            if (id == IDC_LAUNCHER_ADD) {
                if (g_launchers.size() >= MENU_LAUNCHER_LIMIT) {
                    MessageBoxW(
                        hwnd,
                        L"Launcher limit reached.",
                        L"JesterStep",
                        MB_OK | MB_ICONERROR
                    );
                    return 0;
                }

                Launcher launcher{};
                if (!read_launcher_fields(hwnd, &launcher)) {
                    return 0;
                }

                g_launchers.push_back(launcher);
                refresh_launcher_list(hwnd, static_cast<int>(g_launchers.size()) - 1);

                if (g_panel_hwnd) {
                    InvalidateRect(g_panel_hwnd, nullptr, TRUE);
                }

                return 0;
            }

            if (id == IDC_LAUNCHER_UPDATE) {
                int index = get_selected_launcher_index(hwnd);

                if (index < 0 || index >= static_cast<int>(g_launchers.size())) {
                    MessageBoxW(
                        hwnd,
                        L"Select a launcher to update.",
                        L"JesterStep",
                        MB_OK | MB_ICONERROR
                    );
                    return 0;
                }

                Launcher launcher{};
                if (!read_launcher_fields(hwnd, &launcher)) {
                    return 0;
                }

                g_launchers[index] = launcher;
                refresh_launcher_list(hwnd, index);

                if (g_panel_hwnd) {
                    InvalidateRect(g_panel_hwnd, nullptr, TRUE);
                }

                return 0;
            }

            if (id == IDC_LAUNCHER_REMOVE) {
                int index = get_selected_launcher_index(hwnd);

                if (index < 0 || index >= static_cast<int>(g_launchers.size())) {
                    MessageBoxW(
                        hwnd,
                        L"Select a launcher to remove.",
                        L"JesterStep",
                        MB_OK | MB_ICONERROR
                    );
                    return 0;
                }

                g_launchers.erase(g_launchers.begin() + index);

                if (index >= static_cast<int>(g_launchers.size())) {
                    index = static_cast<int>(g_launchers.size()) - 1;
                }

                refresh_launcher_list(hwnd, index);

                if (g_panel_hwnd) {
                    InvalidateRect(g_panel_hwnd, nullptr, TRUE);
                }

                return 0;
            }

            if (id == IDC_LAUNCHER_UP) {
                int index = get_selected_launcher_index(hwnd);

                if (index > 0 && index < static_cast<int>(g_launchers.size())) {
                    Launcher temp = g_launchers[index - 1];
                    g_launchers[index - 1] = g_launchers[index];
                    g_launchers[index] = temp;

                    refresh_launcher_list(hwnd, index - 1);
                }

                return 0;
            }

            if (id == IDC_LAUNCHER_DOWN) {
                int index = get_selected_launcher_index(hwnd);

                if (index >= 0 && index + 1 < static_cast<int>(g_launchers.size())) {
                    Launcher temp = g_launchers[index + 1];
                    g_launchers[index + 1] = g_launchers[index];
                    g_launchers[index] = temp;

                    refresh_launcher_list(hwnd, index + 1);
                }

                return 0;
            }

            if (id == IDC_LAUNCHER_SAVE) {
                if (save_settings()) {
                    MessageBoxW(
                        hwnd,
                        L"Launchers saved.",
                        L"JesterStep",
                        MB_OK | MB_ICONINFORMATION
                    );
                } else {
                    MessageBoxW(
                        hwnd,
                        L"Could not save launchers.",
                        L"JesterStep",
                        MB_OK | MB_ICONERROR
                    );
                }

                return 0;
            }

            if (id == IDC_LAUNCHER_CLOSE) {
                DestroyWindow(hwnd);
                return 0;
            }

            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_NCDESTROY:
            g_launcher_editor_hwnd = nullptr;
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void show_launcher_editor(HWND parent) {
    if (g_launcher_editor_hwnd) {
        ShowWindow(g_launcher_editor_hwnd, SW_SHOW);
        SetForegroundWindow(g_launcher_editor_hwnd);
        return;
    }

    WNDCLASSW wc{};
    wc.lpfnWndProc = launcher_editor_wndproc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = LAUNCHER_EDITOR_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    g_launcher_editor_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        LAUNCHER_EDITOR_CLASS,
        L"JesterStep Launchers",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        540,
        410,
        parent,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (g_launcher_editor_hwnd) {
        ShowWindow(g_launcher_editor_hwnd, SW_SHOW);
        UpdateWindow(g_launcher_editor_hwnd);
    }
}
