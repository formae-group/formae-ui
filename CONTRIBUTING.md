# Contributing to formae-ui

Thank you for helping. formae-ui is shared by several game ports, so every change has to keep the
existing look and behaviour unless the change is about them, and has to stay free of anything
specific to one game. These notes cover the setup, the checks, the style and how a change is merged
and released.

## Setup

Requirements: Windows x64, Visual Studio 2022 (*Desktop development with C++*), CMake 3.20 or
later, Git, and the SDL2 development package for Visual C++
([SDL2-devel-2.x-VC.zip](https://github.com/libsdl-org/SDL/releases)) for the example.

```powershell
git clone --recurse-submodules https://gitlab.com/formae/formae-ui.git
cd formae-ui
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DSDL2_DIR=C:/path/to/SDL2-2.30.9/cmake
cmake --build build --config Release --parallel
```

`examples/imgui` is Dear ImGui v1.91.9b, used by the example and the tests only; the library
itself links whatever ImGui target its consumer provides.

## Checks

Run all of them before opening a merge request:

```powershell
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix build/install
cmake -S tests/package -B build/package -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=${PWD}/build/install
cmake --build build/package --config Release
ctest --test-dir build/package -C Release --output-on-failure
```

| Check | What it covers |
|---|---|
| `formae_ui_smoke` | Every component drawn headless for a few frames; close rules, capture, confirmation |
| `formae_ui_list` | The reorderable list driven headless by keyboard, gamepad and mouse input |
| `formae_ui_selftest` | The example's pages drawn off screen in a hidden window, written to `build/examples/selftest/*.png` |
| `tests/package` | `find_package(formae-ui CONFIG)` from an install, with the consumer's own ImGui target |

Look at the self-test PNGs when you change anything visible, and attach before and after images to
the merge request. Never run the example without `--selftest` on a shared or remote machine: it
opens a visible window.

A change that must not alter what players see (a refactor, a new option with its default unchanged)
should be checked in a consuming game too. DuckTales Recompiled's `DUCKTALES_UI_SELFTEST` writes 16
PNGs of its menu; they must be identical before and after the change.

## Style

- C++17, the library depends on Dear ImGui and the standard library only. No platform headers, no
  backend calls, no game-specific text, setting or name in `include/` or `src/`.
- Four-space indentation, CRLF line endings, UTF-8 (`.editorconfig`, `.gitattributes`).
  `.clang-format` describes the layout.
- Each file opens with a header comment that says what it is for. Public declarations carry a short
  doc comment. No comments inside function bodies.
- Names are `snake_case` for functions and variables, `PascalCase` for types, everything in
  `namespace formae::ui`.
- Every measure derives from the base size and every colour from the `Theme`; no hard-coded pixels
  or colours.
- Keep `README.md`, the matching page in `docs/` and `CHANGELOG.md` current in the same change.

## Commit messages

One logical change per commit. The subject is a short imperative sentence in sentence case, without
a prefix or a trailing period: *Add a percent variant of the slider*. Put the reason and the
evidence (checks run, screenshots compared) in the body.

## Merge requests

Work on a branch and open a merge request against `main` using the default template. A merge
request is accepted when the pipeline passes, the checks above pass, the documentation and the
changelog are updated, and any visible change is shown with screenshots and agreed by a maintainer.

## Releasing

1. Move the *Unreleased* entries of `CHANGELOG.md` under the new version with today's date, and set
   `project(formae_ui VERSION ...)` in `CMakeLists.txt`.
2. Commit (*Release 0.2.0*), then tag with the changelog section as the message:
   `git tag -a v0.2.0 -F <file with the 0.2.0 section>`.
3. Push `main` and the tag. The tag's pipeline builds and tests the release and publishes the
   GitLab release, whose description is the tag message.
4. Update the submodule in the consuming games.
