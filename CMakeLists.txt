# formae-ui: Formae's Dear ImGui menu library (include/formae/ui.h).
#
# add_subdirectory(external/formae-ui) gives the static library formae::ui
# and the function formae_ui_embed_file; an installed copy gives the same
# through find_package(formae-ui CONFIG). The library never builds its own
# Dear ImGui: it links the consumer's ImGui target (FORMAE_UI_IMGUI_TARGET,
# default "imgui"), so a game has exactly one ImGui and one set of ImGui
# configuration macros. A consumer that compiles ImGui into its own target
# sets FORMAE_UI_IMGUI_DIR (and FORMAE_UI_IMGUI_DEFINITIONS) instead.
#
# Options, on only when this is the top-level project:
#   FORMAE_UI_BUILD_EXAMPLES  examples/ (SDL2, ImGui v1.91.9b from examples/imgui)
#   FORMAE_UI_INSTALL         install rules and the package config
cmake_minimum_required(VERSION 3.20)
project(formae_ui VERSION 0.3.0 DESCRIPTION "Dear ImGui menu library for Formae's game ports" LANGUAGES C CXX)

if(CMAKE_VERSION VERSION_LESS 3.21)
    string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)
endif()

set(FORMAE_UI_IMGUI_TARGET "imgui" CACHE STRING "The consumer's Dear ImGui target formae::ui links")
set(FORMAE_UI_IMGUI_DIR "" CACHE PATH "Dear ImGui headers, when the consumer compiles ImGui into its own target")
set(FORMAE_UI_IMGUI_DEFINITIONS "" CACHE STRING "ImGui configuration macros of the consumer's ImGui, with FORMAE_UI_IMGUI_DIR")
option(FORMAE_UI_BUILD_EXAMPLES "Build the SDL2 example and its self-test" ${PROJECT_IS_TOP_LEVEL})
option(FORMAE_UI_INSTALL "Install the library, its headers and its CMake package" ${PROJECT_IS_TOP_LEVEL})

include("${CMAKE_CURRENT_LIST_DIR}/cmake/FormaeUiEmbed.cmake")
include(GNUInstallDirs)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/formae/version.h" @ONLY)

add_library(formae_ui STATIC
    src/context.cpp
    src/widgets.cpp
    src/list.cpp
    src/menu.cpp
    src/about.cpp
    src/first_run.cpp)
add_library(formae::ui ALIAS formae_ui)
set_target_properties(formae_ui PROPERTIES EXPORT_NAME ui OUTPUT_NAME formae-ui)
target_include_directories(formae_ui
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(formae_ui PUBLIC cxx_std_17)
target_compile_definitions(formae_ui PRIVATE _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(formae_ui PRIVATE /utf-8)
endif()

if(FORMAE_UI_IMGUI_DIR)
    if(NOT EXISTS "${FORMAE_UI_IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR "formae-ui: FORMAE_UI_IMGUI_DIR has no imgui.h: ${FORMAE_UI_IMGUI_DIR}")
    endif()
    target_include_directories(formae_ui PUBLIC "$<BUILD_INTERFACE:${FORMAE_UI_IMGUI_DIR}>")
    target_compile_definitions(formae_ui PUBLIC ${FORMAE_UI_IMGUI_DEFINITIONS})
else()
    target_link_libraries(formae_ui PUBLIC "$<BUILD_INTERFACE:${FORMAE_UI_IMGUI_TARGET}>")
    function(_formae_ui_check_imgui)
        if(NOT TARGET "${FORMAE_UI_IMGUI_TARGET}")
            message(FATAL_ERROR "formae-ui: no Dear ImGui target named '${FORMAE_UI_IMGUI_TARGET}'. "
                "Define it (or set FORMAE_UI_IMGUI_TARGET), or set FORMAE_UI_IMGUI_DIR.")
        endif()
    endfunction()
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _formae_ui_check_imgui)
endif()

if(FORMAE_UI_INSTALL)
    include(CMakePackageConfigHelpers)
    set(_formae_ui_cmake_dir "${CMAKE_INSTALL_LIBDIR}/cmake/formae-ui")
    install(TARGETS formae_ui EXPORT formae-ui-targets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(DIRECTORY include/formae DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/formae/version.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/formae")
    install(EXPORT formae-ui-targets NAMESPACE formae:: DESTINATION "${_formae_ui_cmake_dir}")
    configure_package_config_file(cmake/formae-ui-config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/formae-ui-config.cmake"
        INSTALL_DESTINATION "${_formae_ui_cmake_dir}")
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/formae-ui-config-version.cmake"
        COMPATIBILITY SameMinorVersion)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/formae-ui-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/formae-ui-config-version.cmake"
        cmake/FormaeUiEmbed.cmake
        DESTINATION "${_formae_ui_cmake_dir}")
    install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/formae-ui")
endif()

if(FORMAE_UI_BUILD_EXAMPLES)
    enable_testing()
    add_subdirectory(examples)
endif()
