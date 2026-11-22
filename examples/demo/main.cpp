// main.cpp - formae_ui_demo: the menu, About page and first-run screen of a
// made-up game, Demo Quest, built with formae-ui on SDL2 and Dear ImGui's
// SDL_Renderer backend.
//
//   formae_ui_demo                  a window: the first-run screen, then the
//                                   game picture; Escape or Start opens and
//                                   closes the menu over it
//   formae_ui_demo --selftest DIR   a hidden window; every page is drawn off
//                                   screen at a fixed size, 96 DPI and a
//                                   fixed frame time and written to DIR as a
//                                   PNG; the Mods page is also shot after
//                                   scripted key presses that pick a row up
//                                   and move it; exit code 0 when every PNG
//                                   was written
//
// Settings and bindings live in memory only. Everything here is what a game
// supplies: the pages and their texts, the binding names and encoding, the
// logo's texture callbacks and the translation of SDL events into
// formae::ui::InputEvent.
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "formae/ui.h"
#include "formae_logo_png.h"
#include "png_write.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace ui = formae::ui;

namespace {

struct Settings {
    int opacity = 80;
    bool pad_nav = true;
    int difficulty = 1;
    bool subtitles = true;
    int display = 0;
    int scale = 3;
    bool vsync = false;
    bool vsync_started = true;
    int volume = 70;
    bool mods = true;
    bool mods_started = true;
    bool mods_changed = false;
    const char *mods_status = nullptr;
};

const char *const ACTIONS[] = { "Jump", "Attack", "Pause", "Menu" };
const int ACTION_COUNT = 4;
const int MENU_ROW = 3;

Settings settings;
std::vector<ui::ListRow> mods = {
    { "sharper-tiles", "Sharper Tiles", "1.2.0 - Pixel Workshop", "", true },
    { "night-palette", "Night Palette", "0.9.1 - Dusk Collective", "", true },
    { "orchestral-title", "Orchestral Title", "2.0.0 - Brass Section", "", false },
    { "speedrun-timer", "Speedrun Timer", "1.0.0 - Split Second", "Made for a different game version", true },
    { "wide-status-bar", "Wide Status Bar", "0.4.0 - Margins", "", true },
    { "classic-sounds", "Classic Sounds", "1.1.0 - Chiptune Club", "", true },
};
int keys[ACTION_COUNT] = { SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE };
int pads[ACTION_COUNT] = { 1 + SDL_CONTROLLER_BUTTON_A, 1 + SDL_CONTROLLER_BUTTON_X, 1 + SDL_CONTROLLER_BUTTON_START,
                           1 + SDL_CONTROLLER_BUTTON_START };

const char *const BUTTONS[] = { "A", "B", "X", "Y", "Back", "Guide", "Start", "Left stick", "Right stick", "LB", "RB",
                                "D-pad up", "D-pad down", "D-pad left", "D-pad right" };
const char *const AXES[] = { "Left stick X", "Left stick Y", "Right stick X", "Right stick Y", "LT", "RT" };

bool binding_text(int row, int column, char *out, size_t cap)
{
    const int v = column ? pads[row] : keys[row];
    if (!v) {
        std::snprintf(out, cap, "Unbound");
        return false;
    }
    if (!column) std::snprintf(out, cap, "%s", SDL_GetScancodeName((SDL_Scancode)v));
    else if (v >= 100) std::snprintf(out, cap, "%s %s", (v - 100) / 2 < 6 ? AXES[(v - 100) / 2] : "Axis", (v - 100) % 2 ? "+" : "-");
    else std::snprintf(out, cap, "%s", v - 1 < (int)(sizeof BUTTONS / sizeof BUTTONS[0]) ? BUTTONS[v - 1] : "Button");
    return true;
}

const char *binding_assign(int row, int column, const ui::InputEvent *e)
{
    int *table = column ? pads : keys;
    if (!e) {
        table[row] = 0;
        return nullptr;
    }
    int v = 0;
    if (e->type == ui::InputEvent::key_down) v = e->code;
    else if (e->type == ui::InputEvent::button_down) v = 1 + e->code;
    else if (e->type == ui::InputEvent::axis) v = 100 + e->code * 2 + (e->value > 0 ? 1 : 0);
    if (!column && row != MENU_ROW && v == SDL_SCANCODE_ESCAPE) return "Escape stays with the menu; choose another key.";
    for (int r = 0; r < ACTION_COUNT; r++)
        if (r != row && table[r] == v) table[r] = table[row];
    table[row] = v;
    return nullptr;
}

void binding_defaults()
{
    const int k[ACTION_COUNT] = { SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE };
    const int p[ACTION_COUNT] = { 1 + SDL_CONTROLLER_BUTTON_A, 1 + SDL_CONTROLLER_BUTTON_X, 1 + SDL_CONTROLLER_BUTTON_START,
                                  1 + SDL_CONTROLLER_BUTTON_START };
    std::memcpy(keys, k, sizeof keys);
    std::memcpy(pads, p, sizeof pads);
}

ImTextureID create_texture(void *user, const uint32_t *argb, int width, int height)
{
    SDL_Renderer *ren = static_cast<SDL_Renderer *>(user);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!tex) return 0;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
    SDL_UpdateTexture(tex, nullptr, argb, width * 4);
    return (ImTextureID)(intptr_t)tex;
}

void destroy_texture(void *, ImTextureID texture)
{
    SDL_DestroyTexture((SDL_Texture *)(intptr_t)texture);
}

ui::InputEvent translate(const SDL_Event &e)
{
    ui::InputEvent t;
    switch (e.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        const SDL_Scancode sc = e.key.keysym.scancode;
        t.type = e.type == SDL_KEYDOWN ? ui::InputEvent::key_down : ui::InputEvent::key_up;
        t.code = sc;
        t.repeat = e.key.repeat != 0;
        t.cancel = sc == SDL_SCANCODE_ESCAPE;
        t.clear = sc == SDL_SCANCODE_DELETE || sc == SDL_SCANCODE_BACKSPACE;
        t.menu = sc == keys[MENU_ROW];
        break;
    }
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        t.type = e.type == SDL_CONTROLLERBUTTONDOWN ? ui::InputEvent::button_down : ui::InputEvent::button_up;
        t.code = e.cbutton.button;
        t.back = e.cbutton.button == SDL_CONTROLLER_BUTTON_B;
        t.menu = 1 + e.cbutton.button == pads[MENU_ROW];
        break;
    case SDL_CONTROLLERAXISMOTION:
        t.type = ui::InputEvent::axis;
        t.code = e.caxis.axis;
        t.value = e.caxis.value;
        break;
    case SDL_MOUSEBUTTONDOWN:
        t.type = ui::InputEvent::mouse_down;
        break;
    default:
        break;
    }
    return t;
}

SDL_Texture *make_picture(SDL_Renderer *ren)
{
    const int w = 320, h = 180;
    std::vector<uint32_t> px((size_t)w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const float dx = (x - w * 0.5f) / w, dy = (y - h * 0.4f) / h;
            const float glow = 1.0f - (dx * dx + dy * dy) * 2.2f;
            const float g = glow < 0.0f ? 0.0f : glow;
            int r = 34 + (int)(30 * g), gr = 40 + (int)(36 * g), b = 54 + (int)(48 * g);
            if (x % 20 == 0 || y % 20 == 0) {
                r += 6;
                gr += 7;
                b += 9;
            }
            px[(size_t)y * w + x] = 0xFF000000u | (uint32_t)r << 16 | (uint32_t)gr << 8 | (uint32_t)b;
        }
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex) return nullptr;
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    SDL_UpdateTexture(tex, nullptr, px.data(), w * 4);
    return tex;
}

void draw_picture(SDL_Renderer *ren, SDL_Texture *pic, int ow, int oh)
{
    if (!pic) return;
    const SDL_Rect dst = { 0, 0, ow, oh };
    SDL_RenderCopy(ren, pic, nullptr, &dst);
}

float display_dpi(SDL_Window *win)
{
    float ddpi = 0.0f;
    const int display = SDL_GetWindowDisplayIndex(win);
    if (display >= 0 && SDL_GetDisplayDPI(display, &ddpi, nullptr, nullptr) == 0 && ddpi > 0.0f) return ddpi;
    return 96.0f;
}

struct Step {
    bool first_run;
    int tab, category, capture, popup, w, h;
    ui::Status status;
    const char *name;
    const char *keys;
};

ImGuiKey script_key(char c)
{
    switch (c) {
    case 'U': return ImGuiKey_UpArrow;
    case 'D': return ImGuiKey_DownArrow;
    case 'L': return ImGuiKey_LeftArrow;
    case 'R': return ImGuiKey_RightArrow;
    case 'E': return ImGuiKey_Enter;
    default: return ImGuiKey_Space;
    }
}

const Step SCRIPT[] = {
    { false, 0, 0, -1, -1, 1280, 720, ui::Status::none, "menu_01_settings_general" },
    { false, 0, 1, -1, -1, 1280, 720, ui::Status::none, "menu_02_settings_video" },
    { false, 0, 2, -1, -1, 1280, 720, ui::Status::none, "menu_03_settings_audio" },
    { false, 0, 3, -1, -1, 1280, 720, ui::Status::none, "menu_04_settings_controls" },
    { false, 0, 3, 0, -1, 1280, 720, ui::Status::none, "menu_05_settings_controls_capture" },
    { false, 1, 0, -1, -1, 1280, 720, ui::Status::none, "menu_06_about" },
    { false, 0, 1, -1, -1, 960, 540, ui::Status::none, "menu_07_settings_video_540p" },
    { false, 1, 0, -1, -1, 3840, 2160, ui::Status::none, "menu_08_about_2160p" },
    { false, 0, 0, -1, 0, 1280, 720, ui::Status::none, "menu_09_reset" },
    { false, 0, 0, -1, 1, 1280, 720, ui::Status::none, "menu_10_quit" },
    { true, 0, 0, -1, -1, 1280, 720, ui::Status::none, "first_run_1_welcome" },
    { true, 0, 0, -1, -1, 1280, 720, ui::Status::error, "first_run_2_refused" },
    { true, 0, 0, -1, -1, 1280, 720, ui::Status::ok, "first_run_3_verified" },
    { false, 0, 4, -1, -1, 1280, 720, ui::Status::none, "menu_11_settings_mods" },
    { false, 0, 4, -1, -1, 1280, 720, ui::Status::none, "menu_12_settings_mods_moved", "DRREDD" },
};

bool shot(SDL_Renderer *ren, int w, int h, const std::string &dir, const char *name)
{
    std::vector<uint32_t> px((size_t)w * (size_t)h);
    const std::string path = dir + "/" + name + ".png";
    const bool ok = SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_ARGB8888, px.data(), w * 4) == 0 &&
                    png_write(path.c_str(), px.data(), w, h);
    std::printf("[selftest] %s %s (%dx%d)\n", ok ? "wrote" : "could not write", path.c_str(), w, h);
    std::fflush(stdout);
    return ok;
}

}

int main(int argc, char **argv)
{
    std::string selftest;
    for (int i = 1; i + 1 < argc; i++)
        if (!std::strcmp(argv[i], "--selftest")) selftest = argv[i + 1];
    const bool hidden = !selftest.empty();
    std::error_code made;
    if (hidden) std::filesystem::create_directories(selftest, made);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) return 2;
    SDL_Window *win = SDL_CreateWindow("Demo Quest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 640,
                                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (hidden ? SDL_WINDOW_HIDDEN : 0));
    SDL_Renderer *ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (win && !ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        std::fprintf(stderr, "formae_ui_demo: no window: %s\n", SDL_GetError());
        return 2;
    }
    int status = 0;
    {
        ui::Context context;
        ImGui_ImplSDL2_InitForSDLRenderer(win, ren);
        ImGui_ImplSDLRenderer2_Init(ren);
        auto fit = [&](int height, float dpi) {
            if (context.fit(ui::base_size(height, dpi))) ImGui_ImplSDLRenderer2_DestroyFontsTexture();
        };
        ui::Logo logo(formae_logo_png, formae_logo_png_size, { ren, create_texture, destroy_texture });
        SDL_Texture *picture = make_picture(ren);

        ui::BindingTable bindings;
        bindings.columns = { { "Keyboard", ui::Device::keyboard, "Press a key" },
                             { "Controller", ui::Device::gamepad, "Press a button" } };
        bindings.rows = ACTION_COUNT;
        bindings.label = [](int row) { return ACTIONS[row]; };
        bindings.text = binding_text;
        bindings.assign = binding_assign;
        bindings.restore_label = "Restore default controls";
        bindings.restore = binding_defaults;

        ui::About about;
        about.title = "Demo Quest";
        about.version = "0.1.0";
        about.build_date = "2026-11-22";
        about.developer = "Developed by Formae";
        about.logo = &logo;
        about.credits = { { "Demo Quest", "Formae", "MIT" },
                          { "formae-ui", "Formae", "MIT" },
                          { "Dear ImGui", "Omar Cornut", "MIT" },
                          { "SDL2", "Sam Lantinga and contributors", "zlib" } };
        about.disclaimer = "Demo Quest is an example program and not a real game.";

        static const char *const DIFFICULTY[] = { "Easy", "Normal", "Hard" };
        static const char *const DISPLAY[] = { "Windowed", "Borderless full screen", "Exclusive full screen" };
        ui::Menu *menu_ref = nullptr;
        ui::Menu menu(
            { { "Settings",
                { { "General",
                    [] {
                        ui::section("Menu");
                        ui::slider_percent("Menu background opacity", &settings.opacity, 0, 100, 5);
                        ui::toggle("Controller navigation", &settings.pad_nav);
                        ui::section("Game");
                        ui::combo("Difficulty", &settings.difficulty, DIFFICULTY, 3);
                        ui::toggle("Subtitles", &settings.subtitles);
                    } },
                  { "Video",
                    [] {
                        ui::section("Display");
                        ui::combo("Display mode", &settings.display, DISPLAY, 3);
                        ImGui::BeginDisabled(settings.display != 0);
                        ui::slider("Window size", &settings.scale, 1, 8, 1, "%d\xC3\x97");
                        ImGui::EndDisabled();
                        ui::toggle("VSync", &settings.vsync);
                        ui::tag(settings.vsync != settings.vsync_started ? ui::restart_to_apply : nullptr);
                    } },
                  { "Audio",
                    [] {
                        ui::section("Volume");
                        ui::slider_percent("Master volume", &settings.volume, 0, 100, 5);
                    } },
                  { "Controls",
                    [&] {
                        ui::section("Player 1");
                        ui::binding_table(*menu_ref, bindings);
                    } },
                  { "Mods",
                    [] {
                        ui::section("Mods");
                        ui::toggle("Enable mods", &settings.mods);
                        ui::tag(settings.mods != settings.mods_started || settings.mods_changed ? ui::restart_to_apply : nullptr);
                        ui::ListOptions list;
                        list.visible_rows = 5.0f;
                        list.header = "Load order";
                        list.empty = "No mods installed. Put mods in the mods folder, then refresh.";
                        if (ui::reorder_list("##mods", mods, list).kind != ui::ListEdit::none) settings.mods_changed = true;
                        ui::note("Mods higher in the list override the ones below them. Changes apply after a restart.");
                        for (const ui::ListRow &row : mods)
                            if (!row.error.empty()) {
                                ui::note((row.name + " cannot be loaded and stays off.").c_str(), ui::Status::error);
                                break;
                            }
                        ImGui::Spacing();
                        if (ui::inline_button("Open mods folder")) settings.mods_status = "The mods folder would open here.";
                        ImGui::SameLine();
                        if (ui::inline_button("Refresh")) settings.mods_status = "Found 6 mods.";
                        if (settings.mods_status) ui::note(settings.mods_status, ui::Status::ok);
                    } } },
                nullptr },
              { "About", {}, [&] { ui::about_page(about); } } },
            { { "##reset", ui::Icon::reset, false, "Reset the game?", "Reset" },
              { "##quit", ui::Icon::quit, true, "Quit Demo Quest?", "Quit" },
              { "##resume", ui::Icon::resume } });
        menu_ref = &menu;
        ui::FirstRunScreen first_run;
        ui::FirstRunContent welcome;
        welcome.app_title = "Demo Quest";
        welcome.title = "Choose your data";
        welcome.text = "This example asks for a file the way a port asks for its ROM. Choose one, or drop it onto this "
                       "window; it is checked, then kept in your user folder.";
        welcome.primary = "Choose file\xE2\x80\xA6";
        welcome.secondary = "Quit";
        welcome.footer = "Your copy is stored in your user data folder.";

        SDL_Texture *target = nullptr;
        int target_w = 0, target_h = 0;
        size_t step = 0;
        int frames = 0;
        bool in_menu = false, in_first_run = !hidden, quit = false;
        Uint64 verified_at = 0;
        auto apply = [&](const Step &s) {
            menu.select(s.tab, s.category);
            if (s.capture >= 0) menu.begin_capture(&bindings, s.capture, 0);
            else menu.end_capture();
            menu.ask(s.popup);
            if (s.keys) menu.focus_page();
            welcome.status = s.status;
            welcome.busy = s.status == ui::Status::ok;
            welcome.status_text = s.status == ui::Status::ok ? "Verified. Starting the game."
                                  : s.status == ui::Status::error ? "This file is not the one the game needs." : nullptr;
            welcome.detail = s.status == ui::Status::error ? "SHA-256 of this file: 0123456789abcdef\nExpected: fedcba9876543210" : nullptr;
        };
        if (hidden) apply(SCRIPT[0]);
        while (!quit) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) quit = true;
                if (in_menu && !hidden) {
                    const ui::EventResult r = menu.event(translate(ev));
                    if (r == ui::EventResult::close) in_menu = false;
                    if (r != ui::EventResult::pass) continue;
                } else if (!in_first_run && !hidden && ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == keys[MENU_ROW]) {
                    in_menu = true;
                    continue;
                }
                ImGui_ImplSDL2_ProcessEvent(&ev);
            }
            int ow = 0, oh = 0;
            SDL_GetRendererOutputSize(ren, &ow, &oh);
            float dpi = display_dpi(win);
            const Step *s = hidden ? &SCRIPT[step] : nullptr;
            if (s) {
                if (!target || target_w != s->w || target_h != s->h) {
                    if (target) SDL_DestroyTexture(target);
                    target = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, s->w, s->h);
                    target_w = s->w;
                    target_h = s->h;
                }
                if (!target || SDL_SetRenderTarget(ren, target) != 0) {
                    status = 1;
                    break;
                }
                ow = s->w;
                oh = s->h;
                dpi = 96.0f;
            }
            context.set_gamepad_navigation(settings.pad_nav);
            fit(oh, dpi);
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            const bool first = s ? s->first_run : in_first_run;
            if (!first) draw_picture(ren, picture, ow, oh);
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            const int keys_len = s && s->keys ? (int)std::strlen(s->keys) : 0;
            if (s) {
                ImGuiIO &io = ImGui::GetIO();
                io.DisplaySize = ImVec2((float)ow, (float)oh);
                io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
                io.DeltaTime = 1.0f / 60.0f;
                const int k = frames - 2;
                if (k >= 0 && k < 2 * keys_len) io.AddKeyEvent(script_key(s->keys[k / 2]), k % 2 == 0);
            }
            ImGui::NewFrame();
            if (first) {
                const ui::FirstRunAction a = first_run.draw(welcome);
                if (a == ui::FirstRunAction::secondary && !s) quit = true;
                if (a == ui::FirstRunAction::primary && !s) {
                    welcome.status = ui::Status::ok;
                    welcome.status_text = "Verified. Starting the game.";
                    welcome.busy = true;
                    verified_at = SDL_GetTicks64();
                }
            } else if (s || in_menu) {
                const int pressed = menu.draw(settings.opacity);
                if (pressed == 1 && !s) quit = true;
                if ((pressed == 0 || pressed == 2) && !s) in_menu = false;
            }
            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ren);
            if (s && ++frames == 6 + (keys_len ? 2 + 2 * keys_len : 0)) {
                frames = 0;
                if (!shot(ren, ow, oh, selftest, s->name)) status = 1;
                if (++step == sizeof SCRIPT / sizeof SCRIPT[0]) quit = true;
                else apply(SCRIPT[step]);
            }
            if (s) SDL_SetRenderTarget(ren, nullptr);
            if (verified_at && SDL_GetTicks64() - verified_at > 900) {
                in_first_run = false;
                verified_at = 0;
            }
            SDL_RenderPresent(ren);
        }
        if (target) SDL_DestroyTexture(target);
        if (picture) SDL_DestroyTexture(picture);
        logo.release();
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return status;
}
