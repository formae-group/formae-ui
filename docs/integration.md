# Integrating formae-ui in a game

The library draws and decides; the game wires it to its window, its settings, its renderer and its
input. This page describes each connection, with
[DuckTales Recompiled](https://github.com/formae-group/ducktales-recompiled) (`src/ui/app_ui.cpp`) as the
reference integration: a C game on SDL2 and an `SDL_Renderer` (Direct3D 11) whose menu was the
origin of this library.

## Build

DuckTales keeps Dear ImGui as a submodule (`external/imgui`), builds its core as the `imgui`
target, compiles the SDL2 backends into the game and adds formae-ui as a submodule:

```cmake
add_library(imgui STATIC
    "${IMGUI_ROOT}/imgui.cpp" "${IMGUI_ROOT}/imgui_draw.cpp"
    "${IMGUI_ROOT}/imgui_tables.cpp" "${IMGUI_ROOT}/imgui_widgets.cpp")
target_include_directories(imgui PUBLIC "${IMGUI_ROOT}" "${IMGUI_ROOT}/backends")
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
add_subdirectory(external/formae-ui)
target_link_libraries(ducktales PRIVATE imgui formae::ui)
formae_ui_embed_file(ducktales app_about_logo_png "${CMAKE_SOURCE_DIR}/assets/brand/formae.png")
```

The version and build date of the About page come from CMake (`git describe`, `string(TIMESTAMP)`)
in a generated source.

## The menu loop

A port usually pauses the game while its menu is open. DuckTales runs the menu as a modal loop inside
the host's event callback: no machine frame runs, the last picture is drawn under the menu, and the
renderer state is restored when it closes. Each iteration:

1. Poll events. Handle what the game still needs (quit, window resize, controllers added), then
   `menu.event(translate(ev))`, then `ImGui_ImplSDL2_ProcessEvent` when the result is `pass`.
2. Apply the menu's own settings: `context.set_gamepad_navigation(prefs.pad_nav)`.
3. `context.fit(base_size(drawable_height, dpi))`, recreating the font texture when it returns true.
4. Clear and draw the game picture.
5. `NewFrame`, `menu.draw(prefs.opacity)`, act on the returned bar button, `Render`, present.

Make the `Menu` when the menu opens; it then always opens on the first tab.

## Settings

Pages are plain functions or lambdas. Read the game's settings, draw widgets on a copy, and write
back what changed, so every change applies at once and is saved by the game's own code:

```cpp
void page_audio(SDL_Window *win)
{
    AppDisplay d;
    app_display_get(&d);
    ui::section("Volume");
    if (ui::slider_percent("Master volume", &d.volume, 0, 100, 5, app_volume_pending() ? ui::restart_to_apply : nullptr))
        app_display_set(win, &d);
}
```

Lists of options (DuckTales' enhancements) are drawn by iterating the game's table and choosing a
widget per kind. Lambdas capture what a page needs (`[win] { page_graphics(win); }`).

## Textures

The library creates one texture, the About page logo, through `TextureCallbacks`. With an
`SDL_Renderer`:

```cpp
ImTextureID logo_texture(void *user, const uint32_t *argb, int width, int height)
{
    SDL_Texture *tex = SDL_CreateTexture(static_cast<SDL_Renderer *>(user), SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STATIC, width, height);
    if (!tex) return 0;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
    SDL_UpdateTexture(tex, nullptr, argb, width * 4);
    return (ImTextureID)(intptr_t)tex;
}

void logo_release(void *, ImTextureID texture)
{
    SDL_DestroyTexture((SDL_Texture *)(intptr_t)texture);
}

ui::Logo logo(app_about_logo_png, app_about_logo_png_size, { renderer, logo_texture, logo_release });
```

With the Direct3D 11 backend, create a `DXGI_FORMAT_B8G8R8A8_UNORM` texture (the same byte order
as `0xAARRGGBB` on little-endian machines) with a shader resource view, and return the view.
Call `logo.release()` before the renderer is destroyed.

## Input

The menu needs to know what an event means, not where it came from. Translate once:

```cpp
ui::InputEvent translate(const SDL_Event &ev)
{
    ui::InputEvent t;
    switch (ev.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        const SDL_Scancode sc = ev.key.keysym.scancode;
        t.type = ev.type == SDL_KEYDOWN ? ui::InputEvent::key_down : ui::InputEvent::key_up;
        t.code = sc;
        t.repeat = ev.key.repeat != 0;
        t.cancel = sc == SDL_SCANCODE_ESCAPE;
        t.clear = sc == SDL_SCANCODE_DELETE || sc == SDL_SCANCODE_BACKSPACE;
        t.menu = is_menu_key(sc);
        break;
    }
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        t.type = ev.type == SDL_CONTROLLERBUTTONDOWN ? ui::InputEvent::button_down : ui::InputEvent::button_up;
        t.code = ev.cbutton.button;
        t.back = ev.cbutton.button == SDL_CONTROLLER_BUTTON_B;
        t.menu = is_menu_button(ev.cbutton.button);
        break;
    case SDL_CONTROLLERAXISMOTION:
        t.type = ui::InputEvent::axis;
        t.code = ev.caxis.axis;
        t.value = ev.caxis.value;
        break;
    case SDL_MOUSEBUTTONDOWN:
        t.type = ui::InputEvent::mouse_down;
        break;
    default:
        break;
    }
    return t;
}
```

`code` stays in the game's own numbering, so `BindingTable::assign` can store it directly.
DuckTales encodes a controller binding as `1 + button` or `100 + axis * 2 + direction`; an axis
counts once it passes 20000 of 32767.

## The first-run screen

The game owns the window and the file logic; the library owns the layout. DuckTales fills a
`FirstRunContent` each frame from its import state (verified, refused with the file's SHA-256, or
waiting) and opens the Windows file dialog when `draw` returns `primary`.

## Self-test

A port should be able to show its menu without a visible window. DuckTales' `DUCKTALES_UI_SELFTEST`
mode drives the menu with `select`, `begin_capture`, `ask` and `focus_page`, renders every page
off screen and writes PNGs. Comparing them before and after a formae-ui update proves the update
changed nothing visible.
