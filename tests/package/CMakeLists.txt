# tests/package/CMakeLists.txt - a consumer of an installed formae-ui:
# find_package(formae-ui CONFIG) with its own Dear ImGui target (from
# examples/imgui), building and running tests/smoke.cpp against formae::ui.
#
#   cmake --install build --config Release --prefix install
#   cmake -S tests/package -B build-package -DCMAKE_PREFIX_PATH=<repo>/install
#   cmake --build build-package --config Release
#   ctest --test-dir build-package -C Release
cmake_minimum_required(VERSION 3.20)
project(formae_ui_package_test LANGUAGES C CXX)

set(_imgui "${CMAKE_CURRENT_LIST_DIR}/../../examples/imgui")
add_library(imgui STATIC "${_imgui}/imgui.cpp" "${_imgui}/imgui_draw.cpp" "${_imgui}/imgui_tables.cpp" "${_imgui}/imgui_widgets.cpp")
target_include_directories(imgui PUBLIC "${_imgui}")
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

find_package(formae-ui 0.2 CONFIG REQUIRED)

add_executable(formae_ui_package_smoke "${CMAKE_CURRENT_LIST_DIR}/../smoke.cpp")
target_link_libraries(formae_ui_package_smoke PRIVATE formae::ui)
formae_ui_embed_file(formae_ui_package_smoke smoke_png "${CMAKE_CURRENT_LIST_DIR}/../../examples/assets/formae.png")

enable_testing()
add_test(NAME formae_ui_package_smoke COMMAND formae_ui_package_smoke)
