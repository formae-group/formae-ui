// smoke.cpp - formae_ui_smoke: a headless check of the library with no
// window and no renderer backend. It builds the fonts, draws every
// component for a few frames (menu, widgets, binding table and capture,
// confirmation, About page with a logo, first-run screen) and runs the
// close and capture rules, failing on the first broken expectation. It also
// links an embedded file (formae_ui_embed_file) and reads the version header.
#include "formae/ui.h"
#include "smoke_png.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>

namespace ui = formae::ui;

namespace {

int failures = 0;

void expect(bool ok, const char *what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    failures += !ok;
}

ImTextureID fake_texture(void *user, const uint32_t *, int, int)
{
    return (ImTextureID)(intptr_t)++*static_cast<int *>(user);
}

void no_texture(void *, ImTextureID) {}

void frame(const std::function<void()> &draw)
{
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);
    io.DeltaTime = 1.0f / 60.0f;
    ImGui::NewFrame();
    draw();
    ImGui::Render();
}

}

int main()
{
    expect(FORMAE_UI_VERSION_MAJOR == 0 && FORMAE_UI_VERSION_MINOR >= 1, "version header " FORMAE_UI_VERSION);
    expect(smoke_png_size > 8 && !std::memcmp(smoke_png, "\x89PNG", 4), "embedded PNG");
    expect(ui::base_size(720, 96.0f) == 17.0f && ui::base_size(2160, 96.0f) == 51.0f && ui::base_size(480, 192.0f) == 24.0f,
           "base size from height and DPI");

    ui::Context context;
    expect(context.fit(ui::base_size(720, 96.0f)) && !context.fit(17.0f), "fonts built once per base size");
    unsigned char *pixels = nullptr;
    int w = 0, h = 0;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    expect(pixels && w > 0 && h > 0, "font atlas");

    int textures = 0;
    ui::Logo logo(smoke_png, smoke_png_size, { &textures, fake_texture, no_texture });
    int keys[2] = { 4, 0 };
    ui::BindingTable bindings;
    bindings.columns = { { "Keyboard", ui::Device::keyboard, "Press a key" }, { "Controller", ui::Device::gamepad, "Press a button" } };
    bindings.rows = 2;
    bindings.label = [](int row) { return row ? "Attack" : "Jump"; };
    bindings.text = [&](int row, int, char *out, size_t cap) {
        std::snprintf(out, cap, "%d", keys[row]);
        return keys[row] != 0;
    };
    bindings.assign = [&](int row, int, const ui::InputEvent *e) -> const char * {
        if (e && e->code == 41) return "Refused";
        keys[row] = e ? e->code : 0;
        return nullptr;
    };
    bindings.restore_label = "Restore default controls";
    ui::About about;
    about.title = "Smoke";
    about.version = "1.0";
    about.developer = "Developed by Formae";
    about.logo = &logo;
    about.credits = { { "formae-ui", "Formae", "MIT" } };
    about.disclaimer = "Not a game.";
    int level = 50, choice = 1;
    bool flag = true;
    static const char *const names[] = { "One", "Two", "Three" };
    ui::Menu *menu_ref = nullptr;
    ui::Menu menu({ { "Settings",
                      { { "Widgets",
                          [&] {
                              ui::section("Section");
                              ui::slider_percent("Level", &level, 0, 100, 5, ui::restart_to_apply);
                              ui::slider("Scale", &level, 0, 100, 1, "%d\xC3\x97");
                              ui::combo("Choice", &choice, names, 3);
                              ui::toggle("Flag", &flag);
                              ui::button("Button");
                          } },
                        { "Controls", [&] { ui::binding_table(*menu_ref, bindings); } } } },
                    { "About", {}, [&] { ui::about_page(about); } } },
                  { { "##reset", ui::Icon::reset, false, "Reset?", "Reset" }, { "##quit", ui::Icon::quit, true, "Quit?", "Quit" },
                    { "##resume", ui::Icon::resume } });
    menu_ref = &menu;
    for (int tab = 0; tab < 2; tab++)
        for (int category = 0; category < 2; category++) {
            menu.select(tab, category);
            for (int i = 0; i < 3; i++) frame([&] { menu.draw(80); });
        }
    expect(textures == 1 && logo.height() == (int)std::floor(17.0f * 6.5f), "logo texture made once at 6.5 base sizes");

    ui::InputEvent esc;
    esc.type = ui::InputEvent::key_down;
    esc.cancel = true;
    expect(menu.event(esc) == ui::EventResult::close, "Escape closes when nothing is busy");
    ui::InputEvent rb;
    rb.type = ui::InputEvent::button_down;
    rb.menu = true;
    expect(menu.event(rb) == ui::EventResult::close, "the menu button closes");
    ui::InputEvent move;
    expect(menu.event(move) == ui::EventResult::pass, "other events pass to ImGui");

    menu.select(0, 1);
    menu.begin_capture(&bindings, 1, 0);
    frame([&] { menu.draw(80); });
    ui::InputEvent key;
    key.type = ui::InputEvent::key_down;
    key.code = 7;
    expect(menu.event(key) == ui::EventResult::swallow && keys[1] == 7 && !menu.capturing(), "a key binds the captured cell");
    menu.begin_capture(&bindings, 1, 0);
    expect(menu.event(esc) == ui::EventResult::swallow && keys[1] == 7 && !menu.capturing(), "Escape cancels a capture");
    menu.begin_capture(&bindings, 0, 0);
    ui::InputEvent del;
    del.type = ui::InputEvent::key_down;
    del.clear = true;
    expect(menu.event(del) == ui::EventResult::swallow && keys[0] == 0, "Delete clears the captured cell");
    menu.begin_capture(&bindings, 0, 1);
    expect(menu.event(key) == ui::EventResult::swallow && menu.capturing(), "a controller cell ignores keys");
    ui::InputEvent b;
    b.type = ui::InputEvent::button_down;
    b.code = 1;
    b.back = true;
    expect(menu.event(b) == ui::EventResult::swallow && keys[0] == 1 && !menu.capturing(), "a button binds a controller cell");

    menu.ask(1);
    frame([&] { menu.draw(80); });
    frame([&] { menu.draw(80); });
    expect(menu.busy(), "a confirmation makes the menu busy");
    expect(menu.event(esc) == ui::EventResult::pass, "Escape goes to the confirmation, not the menu");

    ui::FirstRunScreen screen;
    ui::FirstRunContent content;
    content.app_title = "Smoke";
    content.title = "Choose your file";
    content.text = "Text";
    content.primary = "Choose file\xE2\x80\xA6";
    content.secondary = "Quit";
    content.status = ui::Status::error;
    content.status_text = "Refused";
    content.detail = "Detail";
    content.footer = "Footer";
    ui::FirstRunAction action = ui::FirstRunAction::none;
    frame([&] { action = screen.draw(content); });
    expect(action == ui::FirstRunAction::none, "first-run screen draws");
    logo.release();
    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
