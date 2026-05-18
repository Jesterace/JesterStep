#include "panel_window.h"

#include "appbar.h"
#include "root_menu.h"
#include "shared_state.h"
#include "task_list.h"

#include <shellapi.h>
#include <string>
#include <windowsx.h>

static const UINT_PTR CLOCK_TIMER_ID = 1;
static const UINT_PTR TASK_TIMER_ID = 2;
static const int LABEL_WIDTH = 130;
static const int CLOCK_WIDTH = 72;
static const int TASK_GAP = 6;
static const int TASK_MIN_WIDTH = 80;
static const int TASK_MAX_WIDTH = 220;

static std::wstring get_time_text() {
    SYSTEMTIME st{};
    GetLocalTime(&st);

    wchar_t buffer[64];
    swprintf_s(buffer, L"%02d:%02d", st.wHour, st.wMinute);
    return buffer;
}

static RECT get_task_area(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    rc.left += LABEL_WIDTH;
    rc.right -= CLOCK_WIDTH;

    return rc;
}

static RECT get_task_rect(HWND hwnd, int index, int task_count) {
    RECT area = get_task_area(hwnd);
    RECT empty{};

    int available = area.right - area.left;
    if (task_count <= 0 || available < TASK_MIN_WIDTH) {
        return empty;
    }

    int width = (available - ((task_count - 1) * TASK_GAP)) / task_count;
    width = clamp_int(width, TASK_MIN_WIDTH, TASK_MAX_WIDTH);

    int left = area.left + index * (width + TASK_GAP);
    int right = left + width;

    if (left >= area.right) {
        return empty;
    }

    if (right > area.right) {
        right = area.right;
    }

    RECT task_rect = area;
    task_rect.left = left;
    task_rect.right = right;
    task_rect.top += 5;
    task_rect.bottom -= 5;

    return task_rect;
}

static int hit_test_task(HWND hwnd, AppState& state, POINT pt) {
    int task_count = static_cast<int>(state.tasks.size());

    for (int i = 0; i < task_count; ++i) {
        RECT task_rect = get_task_rect(hwnd, i, task_count);
        if (!IsRectEmpty(&task_rect) && PtInRect(&task_rect, pt)) {
            return i;
        }
    }

    return -1;
}

static LRESULT CALLBACK panel_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    AppState* state = app_state_from_hwnd(hwnd);

    switch (msg) {
        case WM_CREATE:
            if (state) {
                register_appbar(*state, hwnd);
                refresh_task_list(*state);
            }
            SetTimer(hwnd, CLOCK_TIMER_ID, 1000, nullptr);
            SetTimer(hwnd, TASK_TIMER_ID, 2000, nullptr);
            return 0;

        case WM_TIMER:
            if (state && wparam == TASK_TIMER_ID) {
                refresh_task_list(*state);
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_DISPLAYCHANGE:
            if (state) {
                position_appbar(*state, hwnd);
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_WINDOWPOSCHANGED:
            if (state) {
                notify_appbar_windowpos(*state, hwnd);
            }
            return 0;

        case APPBAR_CALLBACK:
            if (wparam == ABN_POSCHANGED && state) {
                position_appbar(*state, hwnd);
            }
            return 0;

        case WM_RBUTTONUP: {
            POINT pt;
            GetCursorPos(&pt);
            if (state) {
                show_root_menu(*state, hwnd, pt.x, pt.y);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (!state) {
                return 0;
            }

            POINT pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            int index = hit_test_task(hwnd, *state, pt);

            if (index >= 0 && index < static_cast<int>(state->tasks.size())) {
                activate_task_window(state->tasks[index].hwnd);
                refresh_task_list(*state);
                InvalidateRect(hwnd, nullptr, TRUE);
            }

            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            COLORREF bg_color = state ? state->panel_bg_color : RGB(24, 24, 28);
            COLORREF text_color = state ? state->panel_text_color : RGB(230, 230, 230);

            HBRUSH bg = CreateSolidBrush(bg_color);
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, text_color);

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

            if (state) {
                HBRUSH task_brush = CreateSolidBrush(RGB(42, 42, 48));
                HPEN task_pen = CreatePen(PS_SOLID, 1, RGB(70, 70, 78));
                HBRUSH old_brush = (HBRUSH)SelectObject(hdc, task_brush);
                HPEN old_pen = (HPEN)SelectObject(hdc, task_pen);

                int task_count = static_cast<int>(state->tasks.size());
                for (int i = 0; i < task_count; ++i) {
                    RECT task_rect = get_task_rect(hwnd, i, task_count);
                    if (IsRectEmpty(&task_rect)) {
                        continue;
                    }

                    Rectangle(
                        hdc,
                        task_rect.left,
                        task_rect.top,
                        task_rect.right,
                        task_rect.bottom
                    );

                    RECT text_rect = task_rect;
                    text_rect.left += 8;
                    text_rect.right -= 8;

                    DrawTextW(
                        hdc,
                        state->tasks[i].title.c_str(),
                        -1,
                        &text_rect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
                    );
                }

                SelectObject(hdc, old_pen);
                SelectObject(hdc, old_brush);
                DeleteObject(task_pen);
                DeleteObject(task_brush);
            }

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
            KillTimer(hwnd, CLOCK_TIMER_ID);
            KillTimer(hwnd, TASK_TIMER_ID);
            if (state) {
                unregister_appbar(*state, hwnd);
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

HWND create_panel_window(HINSTANCE instance, AppState& state) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = panel_wndproc;
    wc.hInstance = instance;
    wc.lpszClassName = APP_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    return CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        APP_CLASS,
        L"JesterStep",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        state.panel_height,
        nullptr,
        nullptr,
        instance,
        &state
    );
}
