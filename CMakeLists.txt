cmake_minimum_required(VERSION 3.20)

project(JesterStep VERSION 0.1.0 LANGUAGES CXX)

add_executable(JesterStep WIN32
    src/appbar.cpp
    src/color_utils.cpp
    src/launcher_editor_window.cpp
    src/main.cpp
    src/panel_window.cpp
    src/root_menu.cpp
    src/settings_store.cpp
    src/settings_window.cpp
    src/task_list.cpp
)

target_compile_features(JesterStep PRIVATE cxx_std_17)

target_compile_definitions(JesterStep PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(JesterStep PRIVATE
    user32
    gdi32
    shell32
    comdlg32
)
