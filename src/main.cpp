#include <windows.h>
#include <shellapi.h>
#include <string>

static const wchar_t* APP_CLASS = L"JesterStepPanel";
static const UINT APPBAR_CALLBACK = WM_APP + 100;
static const int PANEL_HEIGHT = 36;

static bool g_appbar_registered = false;

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
    abd.uEdge = ABE_TOP;

    abd.rc.left = 0;
    abd.rc.top = 0;
    abd.rc.right = GetSystemMetrics(SM_CXSCREEN);
    abd.rc.bottom = PANEL_HEIGHT;

    SHAppBarMessage(ABM_QUERYPOS, &abd);

    abd.rc.bottom = abd.rc.top + PANEL_HEIGHT;

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

static void show_root_menu(HWND hwnd, int x, int y) {
    HMENU menu = CreatePopupMenu();

    AppendMenuW(menu, MF_STRING, 1001, L"Notepad");
    AppendMenuW(menu, MF_STRING, 1002, L"Explorer");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
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
            position_appbar(hwnd);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case 1004:
            DestroyWindow(hwnd);
            break;
        default:
            break;
    }
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
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
                18, 0, 0, 0,
                FW_NORMAL,
                FALSE, FALSE, FALSE,
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
    WNDCLASSW wc{};
    wc.lpfnWndProc = wndproc;
    wc.hInstance = instance;
    wc.lpszClassName = APP_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        APP_CLASS,
        L"JesterStep",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        PANEL_HEIGHT,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}