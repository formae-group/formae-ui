# examples/CMakeLists.txt - formae_ui_demo: the library's menu, About page
# and first-run screen for a made-up game, on SDL2 and Dear ImGui's
# SDL_Renderer backend. ImGui v1.91.9b comes from the examples/imgui
# submodule (used by this example only); SDL2 from find_package(SDL2), so
# pass -DSDL2_DIR=<folder with sdl2-config.cmake> when CMake cannot find it.
# minimal/ is the README's example (built, never run by the tests).
# "ctest -C Release" runs tests/smoke.cpp and tests/list.cpp (headless)
# and the demo's off-screen self-test (demo/main.cpp).
#
# Under Emscripten (emcmake) SDL2 is Emscripten's port (-sUSE_SDL=2), the
# demo is built as formae_ui_demo.html and FORMAE_UI_WEB_FONTS must name a
# folder with DejaVuSans.ttf and DejaVuSans-Bold.ttf, which are preloaded
# into the page's virtual file system as /fonts (docs/integration.md, Web).
set(FORMAE_UI_WEB_FONTS "" CACHE PATH "Web build: folder with DejaVuSans.ttf and DejaVuSans-Bold.ttf")
set(_imgui "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
if(NOT EXISTS "${_imgui}/imgui.h")
    message(FATAL_ERROR "examples/imgui is missing; run: git submodule update --init examples/imgui")
endif()
if(EMSCRIPTEN)
    foreach(_font DejaVuSans.ttf DejaVuSans-Bold.ttf)
        if(NOT EXISTS "${FORMAE_UI_WEB_FONTS}/${_font}")
            message(FATAL_ERROR "formae-ui web build: no ${_font} in FORMAE_UI_WEB_FONTS ('${FORMAE_UI_WEB_FONTS}'). "
                "Set -DFORMAE_UI_WEB_FONTS=<folder with DejaVuSans.ttf and DejaVuSans-Bold.ttf> "
                "(DejaVu 2.37: https://github.com/dejavu-fonts/dejavu-fonts/releases).")
        endif()
    endforeach()
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_COMPILE_OPTIONS "-sUSE_SDL=2"
        INTERFACE_LINK_OPTIONS "-sUSE_SDL=2")
else()
    find_package(SDL2 CONFIG REQUIRED)
endif()

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
if(EMSCRIPTEN)
    set_target_properties(formae_ui_demo PROPERTIES SUFFIX ".html")
    target_link_options(formae_ui_demo PRIVATE
        "-sALLOW_MEMORY_GROWTH=1"
        "--shell-file=${_imgui}/examples/libs/emscripten/shell_minimal.html"
        "--preload-file=${FORMAE_UI_WEB_FONTS}/DejaVuSans.ttf@/fonts/DejaVuSans.ttf"
        "--preload-file=${FORMAE_UI_WEB_FONTS}/DejaVuSans-Bold.ttf@/fonts/DejaVuSans-Bold.ttf")
endif()
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
