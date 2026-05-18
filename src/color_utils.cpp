#include "color_utils.h"

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

bool parse_hex_color(const std::wstring& text, COLORREF* out_color) {
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

std::wstring color_to_hex(COLORREF color) {
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
