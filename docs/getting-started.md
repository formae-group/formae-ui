# Getting started

This page takes a game from nothing to a working menu. The complete code is in
[examples/minimal/minimal.cpp](../examples/minimal/minimal.cpp) and, with every component, in
[examples/demo/main.cpp](../examples/demo/main.cpp).

## 1. Add the library

```powershell
git submodule add https://github.com/formae-group/formae-ui.git external/formae-ui
git -C external/formae-ui checkout v0.2.1
```

formae-ui links your Dear ImGui target (default name `imgui`). Define it once, with the ImGui
configuration macros your game uses as `PUBLIC` definitions, so the library and the game see the
same ImGui:

```cmake
add_library(imgui STATIC
    external/imgui/imgui.cpp external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp external/imgui/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

add_subdirectory(external/formae-ui)
target_link_libraries(my_game PRIVATE imgui formae::ui)
```

The ImGui backends (`imgui_impl_sdl2.cpp`, `imgui_impl_dx11.cpp`, ...) stay in your game: the
library never calls them.

| CMake variable | Default | Meaning |
|---|---|---|
| `FORMAE_UI_IMGUI_TARGET` | `imgui` | The ImGui target formae::ui links |
| `FORMAE_UI_IMGUI_DIR` | empty | ImGui headers, when ImGui is compiled into your own target instead |
| `FORMAE_UI_IMGUI_DEFINITIONS` | empty | ImGui configuration macros, with `FORMAE_UI_IMGUI_DIR` |
| `FORMAE_UI_BUILD_EXAMPLES` | top level only | The SDL2 example and the tests |
| `FORMAE_UI_INSTALL` | top level only | Install rules and the CMake package |

## 2. Create the context

`formae::ui::Context` creates the ImGui context (keyboard and gamepad navigation on, no
`imgui.ini`) and makes itself current. Initialise your backends after it and shut them down before
it is destroyed:

```cpp
formae::ui::Context ui;
ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
ImGui_ImplSDLRenderer2_Init(renderer);
```

## 3. Fit the size every frame

Before `NewFrame`, compute the base size from the drawable height and the display's DPI. `fit`
rebuilds the fonts and the style only when it changes, and returns true when the backend must
recreate its font texture:

```cpp
if (ui.fit(formae::ui::base_size(drawable_height, display_dpi)))
    ImGui_ImplSDLRenderer2_DestroyFontsTexture();
```

## 4. Describe the menu

A menu is a list of tabs, each with categories (a sidebar entry and the function that draws its
page) or one wide page, and the square buttons at the bar's right:

```cpp
formae::ui::Menu menu(
    { { "Settings", { { "Audio", draw_audio }, { "Video", draw_video } } },
      { "About", {}, draw_about } },
    { { "##quit", formae::ui::Icon::quit, true, "Quit the game?", "Quit" },
      { "##resume", formae::ui::Icon::resume } });
```

Create it when the menu opens, so it opens on the first tab each time.

## 5. Feed events, then draw

Translate each platform event into a `formae::ui::InputEvent` and give it to the menu before ImGui
sees it ([integration.md](integration.md#input) has a complete SDL translation):

```cpp
switch (menu.event(translate(event))) {
case formae::ui::EventResult::close: close_menu(); break;
case formae::ui::EventResult::swallow: break;
case formae::ui::EventResult::pass: ImGui_ImplSDL2_ProcessEvent(&event); break;
}
```

Then draw a frame over your game picture:

```cpp
ImGui::NewFrame();
switch (menu.draw(opacity_percent)) {
case 0: quit_game(); break;
case 1: close_menu(); break;
}
ImGui::Render();
```

`draw` returns the index of the bar button that was pressed, after its confirmation if it has a
question, or -1.

## Next

- [widgets.md](widgets.md): what to put on a page.
- [theming.md](theming.md): colours, fonts and sizes.
- [integration.md](integration.md): settings, textures, input and the first-run screen in a real game.
