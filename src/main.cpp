#include <windows.h>
#include <shellapi.h>
#include <string>
#include <cstdlib>

static const wchar_t* APP_CLASS = L"JesterStepPanel";
static const wchar_t* SETTINGS_CLASS = L"JesterStepSettings";

static const UINT APPBAR_CALLBACK = WM_APP + 100;

static const int IDC_PANEL_POSITION = 2001;
static const int IDC_PANEL_HEIGHT = 2002;
static const int IDC_APPLY = 2003;
static const int IDC_SAVE = 2004;
static const int IDC_CANCEL = 2005;

static HWND g_panel_hwnd = nullptr;
static HWND g_settings_hwnd = nullptr;

static bool g_appbar_registered = false;
static bool g_panel_top = true;
static int g_panel_height = 36;

static int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static std::wstring get_settings_dir() {
    wchar_t appdata[MAX_PATH]{};
    DWORD len = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);

    if (len == 0 || len >= MAX_PATH) {
        return L".";
    }

    return std::wstring(appdata) + L"\\JesterStep";
}

static std::wstring get_settings_path() {
    return get_settings_dir() + L"\\settings.ini";
}

static void ensure_settings_dir() {
    std::wstring dir = get_settings_dir();
    CreateDirectoryW(dir.c_str(), nullptr);
}

static void load_settings() {
    std::wstring path = get_settings_path();

    wchar_t position[32]{};
    GetPrivateProfileStringW(
        L"Panel",
        L"Position",
        L"top",
        position,
        32,
        path.c_str()
    );

    g_panel_top = lstrcmpiW(position, L"bottom") != 0;

    int saved_height = GetPrivateProfileIntW(
        L"Panel",
        L"Height",
        36,
        path.c_str()
    );

    g_panel_height = clamp_int(saved_height, 24, 96);
}

static bool save_settings() {
    ensure_settings_dir();

    std::wstring path = get_settings_path();

    bool ok = true;

    ok = ok && WritePrivateProfileStringW(
        L"Panel",
        L"Position",
        g_panel_top ? L"top" : L"bottom",
        path.c_str()
    );

    wchar_t height_text[32]{};
    swprintf_s(height_text, L"%d", g_panel_height);

    ok = ok && WritePrivateProfileStringW(
        L"Panel",
        L"Height",
        height_text,
        path.c_str()
    );

    return ok;
}

static std::wstring get_time_text() {
    SYSTEMTIME st{};
    GetLocalTime(&st);

    wchar_t buffer[64];
    swprintf_s(buffer, L"%02d:%02d", st.wHour, st.wMinute);
    return buffer;
}

static void launch_app(const wchar_t* app) {
    ShellExecuteW(nullptr, L"open", app, nullptr, nullptr, SW_SHOWNORMAL);
}

static void position_appbar(HWND hwnd) {
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

static bool register_appbar(HWND hwnd) {
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

static void unregister_appbar(HWND hwnd) {
    if (!g_appbar_registered) {
        return;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;

    SHAppBarMessage(ABM_REMOVE, &abd);
    g_appbar_registered = false;
}

static void notify_appbar_windowpos(HWND hwnd) {
    if (!g_appbar_registered) {
        return;
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;

    SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
}

static void refresh_settings_window_controls(HWND hwnd) {
    HWND position_combo = GetDlgItem(hwnd, IDC_PANEL_POSITION);
    HWND height_edit = GetDlgItem(hwnd, IDC_PANEL_HEIGHT);

    if (position_combo) {
        SendMessageW(position_combo, CB_SETCURSEL, g_panel_top ? 0 : 1, 0);
    }

    if (height_edit) {
        wchar_t height_text[32]{};
        swprintf_s(height_text, L"%d", g_panel_height);
        SetWindowTextW(height_edit, height_text);
    }
}

static void apply_settings_from_window(HWND hwnd) {
    HWND position_combo = GetDlgItem(hwnd, IDC_PANEL_POSITION);
    HWND height_edit = GetDlgItem(hwnd, IDC_PANEL_HEIGHT);

    LRESULT selected = SendMessageW(position_combo, CB_GETCURSEL, 0, 0);
    g_panel_top = selected != 1;

    wchar_t height_text[32]{};
    GetWindowTextW(height_edit, height_text, 32);

    int new_height = _wtoi(height_text);
    g_panel_height = clamp_int(new_height, 24, 96);

    refresh_settings_window_controls(hwnd);

    if (g_panel_hwnd) {
        position_appbar(g_panel_hwnd);
        InvalidateRect(g_panel_hwnd, nullptr, TRUE);
    }
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
                (HMENU)IDC_PANEL_POSITION,
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
                (HMENU)IDC_PANEL_HEIGHT,
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
                L"BUTTON",
                L"Apply",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                44,
                132,
                80,
                28,
                hwnd,
                (HMENU)IDC_APPLY,
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Save",
                WS_CHILD | WS_VISIBLE,
                136,
                132,
                80,
                28,
                hwnd,
                (HMENU)IDC_SAVE,
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Cancel",
                WS_CHILD | WS_VISIBLE,
                228,
                132,
                80,
                28,
                hwnd,
                (HMENU)IDC_CANCEL,
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
                apply_settings_from_window(hwnd);

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

static void show_settings_window(HWND parent) {
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
        360,
        220,
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

static void show_root_menu(HWND hwnd, int x, int y) {
    HMENU menu = CreatePopupMenu();

    AppendMenuW(menu, MF_STRING, 1001, L"Notepad");
    AppendMenuW(menu, MF_STRING, 1002, L"Explorer");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 1005, L"Settings...");
    AppendMenuW(menu, MF_STRING, 1003, L"Reload");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 1004, L"Exit JesterStep");

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

    switch (cmd) {
        case 1001:
            launch_app(L"notepad.exe");
            break;
        case 1002:
            launch_app(L"explorer.exe");
            break;
        case 1003:
            reload_saved_settings();
            break;
        case 1004:
            DestroyWindow(hwnd);
            break;
        case 1005:
            show_settings_window(hwnd);
            break;
        default:
            break;
    }
}

static LRESULT CALLBACK panel_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, 1000, nullptr);
            register_appbar(hwnd);
            return 0;

        case WM_TIMER:
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_DISPLAYCHANGE:
            position_appbar(hwnd);
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_WINDOWPOSCHANGED:
            notify_appbar_windowpos(hwnd);
            return 0;

        case APPBAR_CALLBACK:
            if (wparam == ABN_POSCHANGED) {
                position_appbar(hwnd);
            }
            return 0;

        case WM_RBUTTONUP: {
            POINT pt;
            GetCursorPos(&pt);
            show_root_menu(hwnd, pt.x, pt.y);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            HBRUSH bg = CreateSolidBrush(RGB(24, 24, 28));
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(230, 230, 230));

            HFONT font = CreateFontW(
                18,
                0,
                0,
                0,
                FW_NORMAL,
                FALSE,
                FALSE,
                FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                L"Segoe UI"
            );

            HFONT old_font = (HFONT)SelectObject(hdc, font);

            RECT left = rc;
            left.left += 12;
            DrawTextW(
                hdc,
                L"JesterStep",
                -1,
                &left,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE
            );

            std::wstring time = get_time_text();
            RECT right = rc;
            right.right -= 12;
            DrawTextW(
                hdc,
                time.c_str(),
                -1,
                &right,
                DT_RIGHT | DT_VCENTER | DT_SINGLELINE
            );

            SelectObject(hdc, old_font);
            DeleteObject(font);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            unregister_appbar(hwnd);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    load_settings();

    WNDCLASSW wc{};
    wc.lpfnWndProc = panel_wndproc;
    wc.hInstance = instance;
    wc.lpszClassName = APP_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    g_panel_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        APP_CLASS,
        L"JesterStep",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        g_panel_height,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

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