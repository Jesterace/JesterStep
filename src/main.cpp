#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
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
static const int IDC_BG_COLOR = 2006;
static const int IDC_BG_PICK = 2007;
static const int IDC_TEXT_COLOR = 2008;
static const int IDC_TEXT_PICK = 2009;

static HWND g_panel_hwnd = nullptr;
static HWND g_settings_hwnd = nullptr;

static bool g_appbar_registered = false;
static bool g_panel_top = true;
static int g_panel_height = 36;

static COLORREF g_panel_bg_color = RGB(24, 24, 28);
static COLORREF g_panel_text_color = RGB(230, 230, 230);

static int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static int hex_digit_value(wchar_t ch) {
    if (ch >= L'0' && ch <= L'9') {
        return ch - L'0';
    }
    if (ch >= L'a' && ch <= L'f') {
        return 10 + (ch - L'a');
    }
    if (ch >= L'A' && ch <= L'F') {
        return 10 + (ch - L'A');
    }
    return -1;
}

static bool parse_hex_color(const std::wstring& text, COLORREF* out_color) {
    int offset = 0;

    if (text.length() == 7 && text[0] == L'#') {
        offset = 1;
    } else if (text.length() == 6) {
        offset = 0;
    } else {
        return false;
    }

    int values[6]{};

    for (int i = 0; i < 6; ++i) {
        int value = hex_digit_value(text[offset + i]);
        if (value < 0) {
            return false;
        }
        values[i] = value;
    }

    int r = values[0] * 16 + values[1];
    int g = values[2] * 16 + values[3];
    int b = values[4] * 16 + values[5];

    *out_color = RGB(r, g, b);
    return true;
}

static std::wstring color_to_hex(COLORREF color) {
    wchar_t buffer[16]{};
    swprintf_s(
        buffer,
        L"#%02X%02X%02X",
        GetRValue(color),
        GetGValue(color),
        GetBValue(color)
    );
    return buffer;
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

    wchar_t bg_text[32]{};
    GetPrivateProfileStringW(
        L"Theme",
        L"Background",
        L"#18181C",
        bg_text,
        32,
        path.c_str()
    );

    COLORREF parsed_bg{};
    if (parse_hex_color(bg_text, &parsed_bg)) {
        g_panel_bg_color = parsed_bg;
    }

    wchar_t text_color[32]{};
    GetPrivateProfileStringW(
        L"Theme",
        L"Text",
        L"#E6E6E6",
        text_color,
        32,
        path.c_str()
    );

    COLORREF parsed_text{};
    if (parse_hex_color(text_color, &parsed_text)) {
        g_panel_text_color = parsed_text;
    }
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

    std::wstring bg = color_to_hex(g_panel_bg_color);
    std::wstring text = color_to_hex(g_panel_text_color);

    ok = ok && WritePrivateProfileStringW(
        L"Theme",
        L"Background",
        bg.c_str(),
        path.c_str()
    );

    ok = ok && WritePrivateProfileStringW(
        L"Theme",
        L"Text",
        text.c_str(),
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
                (HMENU)IDC_BG_COLOR,
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
                (HMENU)IDC_BG_PICK,
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
                (HMENU)IDC_TEXT_COLOR,
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
                (HMENU)IDC_TEXT_PICK,
                GetModuleHandleW(nullptr),
                nullptr
            );

            CreateWindowExW(
                0,
                L"BUTTON",
                L"Apply",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                44,
                224,
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
                156,
                224,
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
                268,
                224,
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
        420,
        320,
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

            HBRUSH bg = CreateSolidBrush(g_panel_bg_color);
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_panel_text_color);

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