# examples/CMakeLists.txt - formae_ui_demo: the library's menu, About page
# and first-run screen for a made-up game, on SDL2 and Dear ImGui's
# SDL_Renderer backend. ImGui v1.91.9b comes from the examples/imgui
# submodule (used by this example only); SDL2 from find_package(SDL2), so
# pass -DSDL2_DIR=<folder with sdl2-config.cmake> when CMake cannot find it.
# minimal/ is the README's example (built, never run by the tests).
# "ctest -C Release" runs tests/smoke.cpp and tests/list.cpp (headless)
# and the demo's off-screen self-test (demo/main.cpp).
set(_imgui "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
if(NOT EXISTS "${_imgui}/imgui.h")
    message(FATAL_ERROR "examples/imgui is missing; run: git submodule update --init examples/imgui")
endif()
find_package(SDL2 CONFIG REQUIRED)

if(NOT TARGET imgui)
    add_library(imgui STATIC
        "${_imgui}/imgui.cpp"
        "${_imgui}/imgui_draw.cpp"
        "${_imgui}/imgui_tables.cpp"
        "${_imgui}/imgui_widgets.cpp"
        "${_imgui}/backends/imgui_impl_sdl2.cpp"
        "${_imgui}/backends/imgui_impl_sdlrenderer2.cpp")
    target_include_directories(imgui PUBLIC "${_imgui}" "${_imgui}/backends")
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
    target_link_libraries(imgui PUBLIC SDL2::SDL2)
endif()

add_executable(formae_ui_demo demo/main.cpp demo/png_write.cpp)
target_link_libraries(formae_ui_demo PRIVATE formae::ui imgui SDL2::SDL2)
target_compile_definitions(formae_ui_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
formae_ui_embed_file(formae_ui_demo formae_logo_png "${CMAKE_CURRENT_SOURCE_DIR}/assets/formae.png")
if(WIN32)
    add_custom_command(TARGET formae_ui_demo POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:formae_ui_demo>"
        VERBATIM)
endif()

add_executable(formae_ui_minimal minimal/minimal.cpp)
target_link_libraries(formae_ui_minimal PRIVATE formae::ui imgui SDL2::SDL2)

add_executable(formae_ui_smoke "${PROJECT_SOURCE_DIR}/tests/smoke.cpp")
target_link_libraries(formae_ui_smoke PRIVATE formae::ui)
formae_ui_embed_file(formae_ui_smoke smoke_png "${CMAKE_CURRENT_SOURCE_DIR}/assets/formae.png")

add_executable(formae_ui_list "${PROJECT_SOURCE_DIR}/tests/list.cpp")
target_link_libraries(formae_ui_list PRIVATE formae::ui)
if(MSVC)
    target_compile_options(formae_ui_demo PRIVATE /utf-8)
    target_compile_options(formae_ui_minimal PRIVATE /utf-8)
    target_compile_options(formae_ui_smoke PRIVATE /utf-8)
    target_compile_options(formae_ui_list PRIVATE /utf-8)
endif()

add_test(NAME formae_ui_smoke COMMAND formae_ui_smoke)
add_test(NAME formae_ui_list COMMAND formae_ui_list)
add_test(NAME formae_ui_selftest
    COMMAND formae_ui_demo --selftest "${CMAKE_CURRENT_BINARY_DIR}/selftest")
