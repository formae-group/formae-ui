# Changelog

All notable changes to formae-ui are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- The formae-ui logo (`assets/logo/`: SVG master, 512 and 256 px PNG) at the top of the README.

## [0.1.0] - 2026-11-22

### Added

- First release, extracted from the DuckTales Recompiled menu with an identical look.
- `Context`: ImGui context with keyboard and gamepad navigation, `Theme` colours, system UI fonts
  (Segoe UI) with ImGui's font as the fallback, and the base size from the drawable height and DPI.
- Menu shell (`Menu`): tabs, reset, quit and resume icon buttons drawn with ImDrawList, a category
  sidebar, the page, a backdrop of configurable opacity, confirmations, and the close rules (the menu
  shortcut always closes; Escape and B unless a popup or active item needs them).
- Widgets: `section`, `tag` and `restart_to_apply`, `toggle`, `combo`, `slider` and
  `slider_percent` with - and + buttons that repeat while held, `button`, `pick_button`,
  `icon_button`, `confirm`.
- Input binding table (`BindingTable`, `binding_table`) with capture: Escape cancels, Delete clears,
  refusals shown for four seconds, the menu held until the bound button is released.
- About page (`About`, `about_page`) with a developer `Logo` decoded by stb_image and created
  through the game's `TextureCallbacks`.
- First-run screen (`FirstRunScreen`) for ROM or data import.
- CMake target `formae::ui` for `add_subdirectory` and `find_package(formae-ui CONFIG)`, linking
  the consumer's ImGui target; `formae_ui_embed_file`; `formae/version.h`.
- SDL2 example with an off-screen self-test, a headless smoke test, a package consumer test and a
  GitLab CI pipeline on Windows.

[Unreleased]: https://gitlab.com/formae/formae-ui/-/compare/v0.1.0...main
[0.1.0]: https://gitlab.com/formae/formae-ui/-/tags/v0.1.0
