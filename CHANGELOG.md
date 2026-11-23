# Changelog

All notable changes to formae-ui are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.1] - 2026-11-23

### Changed

- The project moved to GitHub: https://github.com/formae-group/formae-ui. CI is GitHub Actions
  (`.github/workflows/ci.yml`): it builds and tests the library, the example and the installed
  package on Windows x64 (MSVC) and Linux x64 (GCC), and a tag publishes the GitHub release with
  the installed library for both. The GitLab pipeline and templates are replaced by GitHub's.
- Linux x64 is a supported, tested platform (the library code needed no change).
- One code layout for every C++ and Python source: `.clang-format` (4 spaces, 120 columns, braces
  always, no one-line statements) and Black, applied by `tools/format.py` (`--check` to verify).

## [0.2.0] - 2026-11-22

### Added

- `reorder_list` with `ListRow`, `ListEdit` and `ListOptions`: rows in priority order (mods,
  packs) with up and down buttons drawn with ImDrawList, an enable checkbox, dimmed details and an
  error line that disables the checkbox. Rows move with the buttons (the focus stays on the moved
  row), Ctrl or Alt with Up and Down, a pick-up with A, Enter or Space (Up and Down then move the
  row; A, Enter, Space, B or Escape put it down, and the menu stays busy so it does not close) or a
  mouse drag. An optional scrolling region that navigation enters and leaves like the page, a
  caption and an empty text. Moves and toggles are applied in place and reported once per frame.
- `note`: a paragraph wrapped to the page in the dimmed, ok or error colour.
- `inline_button`: a button as wide as its label, for several on one line.
- The formae-ui logo (`assets/logo/`: SVG master, 512 and 256 px PNG) at the top of the README.
- Example: a Settings > Mods page, shot by the self-test as drawn and after scripted key presses
  that pick a row up and move it (`menu_11_settings_mods`, `menu_12_settings_mods_moved`).
- `tests/list.cpp` (`formae_ui_list`): the list driven headless by keyboard, gamepad and mouse
  input.

### Changed

- `Status` is declared before the list widgets, which use it too; its values are unchanged.
- The example's self-test runs at a fixed frame time, so the confirmation shots, whose dimming fades
  in, are the same on every run.

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

[Unreleased]: https://github.com/formae-group/formae-ui/compare/v0.2.1...main
[0.2.1]: https://github.com/formae-group/formae-ui/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/formae-group/formae-ui/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/formae-group/formae-ui/releases/tag/v0.1.0
