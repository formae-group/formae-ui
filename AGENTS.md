# AGENTS.md

Instructions for coding agents working in this repository. Read [README.md](README.md),
[CONTRIBUTING.md](CONTRIBUTING.md) and the `docs/` page for the area you are changing first.

## Project

formae-ui is the Dear ImGui menu library of Formae's game ports (first consumer: DuckTales
Recompiled). The library draws and decides; the game owns the window, the ImGui backends, the event
loop, its settings and its texts.

## Hard rules

- The library (`include/`, `src/`) depends on Dear ImGui and the C++ standard library only: no
  SDL, Windows or renderer headers, and no game-specific text, setting or name.
- Never vendor Dear ImGui into the library. `examples/imgui` (v1.91.9b) is for the example and the
  tests only; consumers link their own `imgui` target.
- Do not change the default look or behaviour unless that is the task: games compare their menu
  pixel for pixel across library updates.
- Never open a visible window on the developer's desktop: run the example only with `--selftest`.
- Windows x64 with Visual Studio 2022 is the supported platform.
- Do not push to `main`, tag or publish a release without being asked.

## Checks

| Change | Command |
|---|---|
| Any | `cmake --build build --config Release --parallel` then `ctest --test-dir build -C Release --output-on-failure` |
| CMake, install, package config | the `tests/package` steps in CONTRIBUTING.md |
| Anything visible | look at `build/examples/selftest/*.png` before and after |

## Conventions

- C++17, four-space indent, CRLF line endings, UTF-8.
- Each file opens with a header comment naming its purpose. Public declarations get a short doc
  comment. No comments inside function bodies.
- `snake_case` functions and variables, `PascalCase` types, `namespace formae::ui`.
- Measures from the base size, colours from the `Theme`.
- Commit subjects are short imperative sentences in sentence case, no prefix, no trailing period.
- Keep the README, the matching `docs/` page and `CHANGELOG.md` current in the same change.
- Report what you verified and what you did not.
