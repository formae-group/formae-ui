// list.cpp - formae_ui_list: a headless check of reorder_list driven by
// injected keyboard, gamepad and mouse input inside a Menu, with no window
// and no renderer backend. It checks that the focused up or down button
// keeps moving the same row, Ctrl with the arrows moves the focused row
// without moving the focus, the checkbox toggles, a row picked up with A or
// Enter follows the D-pad or the arrows and keeps the menu busy until B or
// Escape puts it down, a mouse drag moves a row one place, Down crosses
// from the page into a scrolling list (beside the menu's sidebar) and out of
// it again, and a row moved in a scrolling list is scrolled into view.
#include "formae/ui.h"
#include "imgui_internal.h"

#include <cstdio>
#include <string>
#include <vector>

namespace ui = formae::ui;

namespace {

int failures = 0;

void expect(bool ok, const char *what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    std::fflush(stdout);
    failures += !ok;
}

std::string order(const std::vector<ui::ListRow> &rows)
{
    std::string ids;
    for (const ui::ListRow &row : rows) {
        ids += row.id;
    }
    return ids;
}

bool is(const std::vector<ui::ListEdit> &edits, ui::ListEdit::Kind kind, int row, int to = -1)
{
    return edits.size() == 1 && edits[0].kind == kind && edits[0].row == row && edits[0].to == to;
}

} // namespace

int main()
{
    ui::Context context;
    context.fit(17.0f);
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    std::vector<ui::ListRow> rows = { { "a", "Alpha", "1.0.0 - Someone", "", true },
                                      { "b", "Beta", "", "", false },
                                      { "c", "Gamma", "", "Made for a different game version", true },
                                      { "d", "Delta", "", "", true } };
    std::vector<ui::ListRow> many;
    for (int i = 0; i < 8; i++) {
        many.push_back({ std::to_string(i), "Row " + std::to_string(i), "", "", true });
    }
    ui::ListOptions scrolling;
    scrolling.visible_rows = 2.0f;
    std::vector<ui::ListEdit> edits;
    ImRect last;
    float scroll = 0.0f;
    bool above = false;
    int below = 0;
    ui::Menu menu({ { "Mods",
                      { { "List",
                          [&] {
                              const ui::ListEdit e = ui::reorder_list("##mods", rows);
                              if (e.kind != ui::ListEdit::none) {
                                  edits.push_back(e);
                              }
                              last = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                          } },
                        { "Scrolling",
                          [&] {
                              ui::toggle("Above", &above);
                              const ui::ListEdit e = ui::reorder_list("##many", many, scrolling);
                              if (e.kind != ui::ListEdit::none) {
                                  edits.push_back(e);
                              }
                              const ImVector<ImGuiWindow *> &children = ImGui::GetCurrentWindow()->DC.ChildWindows;
                              scroll = children.Size ? children.back()->Scroll.y : -1.0f;
                              below += ui::inline_button("Below");
                          } } } } },
                  { { "##resume", ui::Icon::resume } });
    auto frame = [&] {
        io.DisplaySize = ImVec2(1280, 720);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();
        menu.draw(80);
        ImGui::Render();
    };
    auto tap = [&](ImGuiKey key) {
        edits.clear();
        io.AddKeyEvent(key, true);
        frame();
        io.AddKeyEvent(key, false);
        frame();
    };
    auto modifier = [&](bool down) {
        io.AddKeyEvent(ImGuiMod_Ctrl, down);
        io.AddKeyEvent(ImGuiKey_LeftCtrl, down);
        frame();
    };

    menu.select(0, 0);
    menu.focus_page();
    frame();
    frame();
    tap(ImGuiKey_Enter);
    expect(is(edits, ui::ListEdit::moved, 0, 1) && order(rows) == "bacd", "the down button moves its row");
    tap(ImGuiKey_Enter);
    expect(is(edits, ui::ListEdit::moved, 1, 2) && order(rows) == "bcad", "the focus stays on the moved row's button");

    modifier(true);
    tap(ImGuiKey_UpArrow);
    expect(is(edits, ui::ListEdit::moved, 2, 1) && order(rows) == "bacd", "Ctrl+Up moves the focused row");
    tap(ImGuiKey_UpArrow);
    expect(is(edits, ui::ListEdit::moved, 1, 0) && order(rows) == "abcd", "Ctrl+Up keeps the focus on the moved row");
    modifier(false);

    tap(ImGuiKey_RightArrow);
    tap(ImGuiKey_Space);
    expect(is(edits, ui::ListEdit::toggled, 0) && !rows[0].enabled, "Space on the checkbox toggles the row");
    tap(ImGuiKey_Space);
    expect(is(edits, ui::ListEdit::toggled, 0) && rows[0].enabled, "and toggles it back");

    tap(ImGuiKey_RightArrow);
    tap(ImGuiKey_GamepadFaceDown);
    expect(edits.empty() && menu.busy(), "A on the name picks the row up and makes the menu busy");
    tap(ImGuiKey_GamepadDpadDown);
    expect(is(edits, ui::ListEdit::moved, 0, 1) && order(rows) == "bacd", "the D-pad moves a picked-up row");
    tap(ImGuiKey_GamepadDpadDown);
    expect(is(edits, ui::ListEdit::moved, 1, 2) && order(rows) == "bcad", "past a row with an error");
    ui::InputEvent b;
    b.type = ui::InputEvent::button_down;
    b.back = true;
    ui::InputEvent esc;
    esc.type = ui::InputEvent::key_down;
    esc.cancel = true;
    expect(menu.busy() && menu.event(b) == ui::EventResult::pass && menu.event(esc) == ui::EventResult::pass,
           "B and Escape go to ImGui while a row is picked up");
    tap(ImGuiKey_GamepadFaceRight);
    expect(edits.empty() && !menu.busy(), "B puts the row down");
    tap(ImGuiKey_GamepadDpadUp);
    expect(edits.empty() && order(rows) == "bcad", "the D-pad navigates again once the row is down");

    tap(ImGuiKey_Enter);
    expect(edits.empty() && menu.busy(), "Enter on the name picks the row up");
    tap(ImGuiKey_UpArrow);
    expect(is(edits, ui::ListEdit::moved, 1, 0) && order(rows) == "cbad", "Up moves a row picked up with the keyboard");
    tap(ImGuiKey_Escape);
    expect(edits.empty() && !menu.busy(), "Escape puts the row down");

    edits.clear();
    const float pitch = last.GetHeight();
    io.AddMousePosEvent(last.GetCenter().x, last.GetCenter().y);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    for (int i = 1; i <= 4; i++) {
        io.AddMousePosEvent(last.GetCenter().x, last.GetCenter().y - pitch * 1.2f * (float)i / 4.0f);
        frame();
    }
    io.AddMouseButtonEvent(0, false);
    frame();
    expect(is(edits, ui::ListEdit::moved, 3, 2) && order(rows) == "cbda", "dragging a row's name moves it one place");

    menu.select(0, 1);
    menu.focus_page();
    frame();
    frame();
    expect(scroll == 0.0f, "a scrolling list starts at the top");
    tap(ImGuiKey_DownArrow);
    tap(ImGuiKey_Enter);
    expect(is(edits, ui::ListEdit::moved, 0, 1) && !above && !below,
           "Down from the page enters a scrolling list beside a sidebar");
    for (int i = 0; i < 4; i++) {
        tap(ImGuiKey_Enter);
    }
    expect(order(many) == "12345067" && scroll > 0.0f, "a row moved in a scrolling list is scrolled into view");
    for (int i = 0; i < 3; i++) {
        tap(ImGuiKey_DownArrow);
    }
    tap(ImGuiKey_Space);
    expect(below == 1 && !above, "Down from the last row leaves the list for the page");

    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
