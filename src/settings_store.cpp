#include "settings_store.h"

#include "color_utils.h"
#include "shared_state.h"

#include <windows.h>

static const wchar_t* RUN_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* RUN_VALUE_NAME = L"JesterStep";

static std::wstring get_settings_dir() {
    wchar_t appdata[MAX_PATH]{};
    DWORD len = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);

    if (len == 0 || len >= MAX_PATH) {
        return L".";
    }

    return std::wstring(appdata) + L"\\JesterStep";
}

std::wstring get_settings_path() {
    return get_settings_dir() + L"\\settings.ini";
}

static void ensure_settings_dir() {
    std::wstring dir = get_settings_dir();
    CreateDirectoryW(dir.c_str(), nullptr);
}

static std::wstring read_ini_string(
    const std::wstring& path,
    const wchar_t* section,
    const wchar_t* key,
    const wchar_t* fallback
) {
    wchar_t buffer[512]{};

    GetPrivateProfileStringW(
        section,
        key,
        fallback,
        buffer,
        512,
        path.c_str()
    );

    return buffer;
}

static bool read_ini_bool(
    const std::wstring& path,
    const wchar_t* section,
    const wchar_t* key,
    bool fallback
) {
    std::wstring fallback_text = fallback ? L"true" : L"false";
    std::wstring value = read_ini_string(path, section, key, fallback_text.c_str());

    return
        lstrcmpiW(value.c_str(), L"true") == 0 ||
        lstrcmpiW(value.c_str(), L"1") == 0 ||
        lstrcmpiW(value.c_str(), L"yes") == 0 ||
        lstrcmpiW(value.c_str(), L"on") == 0;
}

static bool get_current_executable_path(std::wstring& path) {
    wchar_t buffer[MAX_PATH]{};
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

    if (len == 0 || len >= MAX_PATH) {
        return false;
    }

    path = buffer;
    return true;
}

static bool update_startup_registration(bool enabled) {
    HKEY key{};
    LONG result = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        RUN_KEY,
        0,
        nullptr,
        0,
        KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    bool ok = true;

    if (enabled) {
        std::wstring path;
        ok = get_current_executable_path(path);

        if (ok) {
            std::wstring command = L"\"" + path + L"\"";
            result = RegSetValueExW(
                key,
                RUN_VALUE_NAME,
                0,
                REG_SZ,
                reinterpret_cast<const BYTE*>(command.c_str()),
                static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t))
            );
            ok = result == ERROR_SUCCESS;
        }
    } else {
        result = RegDeleteValueW(key, RUN_VALUE_NAME);
        ok = result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
    }

    RegCloseKey(key);
    return ok;
}

static void load_default_launchers(AppState& state) {
    state.launchers.clear();

    state.launchers.push_back({L"Notepad", L"notepad.exe"});
    state.launchers.push_back({L"Explorer", L"explorer.exe"});
}

static void load_launchers(AppState& state, const std::wstring& path) {
    state.launchers.clear();

    int count = GetPrivateProfileIntW(
        L"Launchers",
        L"Count",
        0,
        path.c_str()
    );

    count = clamp_int(count, 0, MENU_LAUNCHER_LIMIT);

    if (count <= 0) {
        load_default_launchers(state);
        return;
    }

    for (int i = 1; i <= count; ++i) {
        wchar_t section[32]{};
        swprintf_s(section, L"Launcher%d", i);

        std::wstring name = read_ini_string(path, section, L"Name", L"");
        std::wstring command = read_ini_string(path, section, L"Command", L"");

        if (!name.empty() && !command.empty()) {
            state.launchers.push_back({name, command});
        }
    }

    if (state.launchers.empty()) {
        load_default_launchers(state);
    }
}

void load_settings(AppState& state) {
    std::wstring path = get_settings_path();

    state.start_with_windows = read_ini_bool(
        path,
        L"General",
        L"StartWithWindows",
        false
    );

    wchar_t position[32]{};
    GetPrivateProfileStringW(
        L"Panel",
        L"Position",
        L"top",
        position,
        32,
        path.c_str()
    );

    state.panel_top = lstrcmpiW(position, L"bottom") != 0;

    int saved_height = GetPrivateProfileIntW(
        L"Panel",
        L"Height",
        36,
        path.c_str()
    );

    state.panel_height = clamp_int(saved_height, 24, 96);

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
        state.panel_bg_color = parsed_bg;
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
        state.panel_text_color = parsed_text;
    }

    wchar_t accent_color[32]{};
    GetPrivateProfileStringW(
        L"Theme",
        L"Accent",
        L"#409CFF",
        accent_color,
        32,
        path.c_str()
    );

    COLORREF parsed_accent{};
    if (parse_hex_color(accent_color, &parsed_accent)) {
        state.accent_color = parsed_accent;
    }

    load_launchers(state, path);
}

bool save_settings(const AppState& state) {
    ensure_settings_dir();

    std::wstring path = get_settings_path();

    bool ok = true;

    ok = ok && WritePrivateProfileStringW(
        L"General",
        L"StartWithWindows",
        state.start_with_windows ? L"true" : L"false",
        path.c_str()
    );

    bool startup_ok = update_startup_registration(state.start_with_windows);

    ok = ok && WritePrivateProfileStringW(
        L"Panel",
        L"Position",
        state.panel_top ? L"top" : L"bottom",
        path.c_str()
    );

    wchar_t height_text[32]{};
    swprintf_s(height_text, L"%d", state.panel_height);

    ok = ok && WritePrivateProfileStringW(
        L"Panel",
        L"Height",
        height_text,
        path.c_str()
    );

    std::wstring bg = color_to_hex(state.panel_bg_color);
    std::wstring text = color_to_hex(state.panel_text_color);
    std::wstring accent = color_to_hex(state.accent_color);

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

    ok = ok && WritePrivateProfileStringW(
        L"Theme",
        L"Accent",
        accent.c_str(),
        path.c_str()
    );

    wchar_t launcher_count[32]{};
    swprintf_s(launcher_count, L"%d", static_cast<int>(state.launchers.size()));

    ok = ok && WritePrivateProfileStringW(
        L"Launchers",
        L"Count",
        launcher_count,
        path.c_str()
    );

    for (size_t i = 0; i < state.launchers.size(); ++i) {
        wchar_t section[32]{};
        swprintf_s(section, L"Launcher%zu", i + 1);

        ok = ok && WritePrivateProfileStringW(
            section,
            L"Name",
            state.launchers[i].name.c_str(),
            path.c_str()
        );

        ok = ok && WritePrivateProfileStringW(
            section,
            L"Command",
            state.launchers[i].command.c_str(),
            path.c_str()
        );
    }

    return ok && startup_ok;
}
