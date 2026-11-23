// formae/ui.h - formae-ui: the in-game menu, the About page and the
// first-run screen of Formae's game ports, drawn with Dear ImGui.
//
// The library only draws and decides. The game owns the window, the ImGui
// platform and renderer backends, the event loop, its settings and every
// text it shows. One frame of the menu is:
//
//   formae::ui::Context ui;                 ImGui context, theme, fonts
//   ui.fit(formae::ui::base_size(h, dpi));  rebuild fonts when the size changes
//   menu.event(translated_event);           for each input event, before ImGui
//   ImGui::NewFrame();
//   menu.draw(opacity_percent);             the shell and the chosen page
//   ImGui::Render();
//
// Pages are callbacks that call the widgets below (section, toggle, combo,
// slider, button, note, reorder_list, binding_table, about_page). Sizes are measured in the
// base size (Fonts::base); colours come from the Theme, whose defaults are
// DuckTales Recompiled's look. See docs/widgets.md for every component.
#pragma once

#include "formae/version.h"
#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace formae::ui {

// Every colour the library draws with, as ImGui IM_COL32 values. Override
// fields before passing a Theme to Context; the defaults are the reference look.
struct Theme {
    ImU32 accent = IM_COL32(242, 179, 61, 255);       // chosen tab and category, rules, checks, slider grab
    ImU32 accent_hi = IM_COL32(252, 200, 102, 255);   // accent while hovered or held
    ImU32 on_accent = IM_COL32(26, 19, 6, 255);       // text on accent fills
    ImU32 backdrop = IM_COL32(11, 13, 18, 255);       // behind the menu (its alpha comes from the opacity)
    ImU32 text = IM_COL32(236, 239, 243, 255);        // body text
    ImU32 text_dim = IM_COL32(150, 158, 171, 255);    // section titles, secondary text
    ImU32 text_off = IM_COL32(98, 105, 118, 255);     // disabled text, unbound bindings, footers
    ImU32 line = IM_COL32(255, 255, 255, 34);         // separators and borders
    ImU32 soft_hover = IM_COL32(255, 255, 255, 22);   // unchosen tab or category under the pointer
    ImU32 soft_active = IM_COL32(255, 255, 255, 40);  // unchosen tab or category while held
    ImU32 danger = IM_COL32(172, 56, 50, 255);        // destructive bar button (Quit)
    ImU32 danger_hi = IM_COL32(204, 74, 66, 255);     // destructive button hovered or held
    ImU32 error_text = IM_COL32(239, 112, 100, 255);  // refusals and errors
    ImU32 ok_text = IM_COL32(126, 205, 140, 255);     // success messages
    ImU32 popup = IM_COL32(24, 28, 36, 252);          // combo lists and dialogs
    ImU32 frame = IM_COL32(40, 46, 58, 255);          // slider, combo and checkbox frames
    ImU32 frame_hovered = IM_COL32(54, 62, 77, 255);  // frame under the pointer
    ImU32 frame_active = IM_COL32(66, 76, 94, 255);   // frame while held
    ImU32 button = IM_COL32(44, 51, 64, 255);         // buttons
    ImU32 button_hovered = IM_COL32(60, 70, 87, 255); // button under the pointer
    ImU32 button_active = IM_COL32(76, 88, 108, 255); // button while held
    ImU32 scrollbar = IM_COL32(255, 255, 255, 40);    // scrollbar grab
    ImU32 scrollbar_hovered = IM_COL32(255, 255, 255, 64);
    ImU32 scrollbar_active = IM_COL32(255, 255, 255, 90);
    ImU32 row_alt = IM_COL32(255, 255, 255, 7); // alternate table rows (credits, reorderable lists)
    ImU32 modal_dim = IM_COL32(0, 0, 0, 120);   // behind a confirmation
    int header_alpha = 64;                      // alpha of the accent behind a selected combo entry
    int selection_alpha = 90;                   // alpha of the accent behind selected text
};

// The TrueType files Context loads: a regular face and a heavier one for
// titles and emphasis. An empty or missing file falls back to ImGui's font.
struct FontFiles {
    std::string regular;
    std::string semibold;
};

// The fonts built for the current base size.
struct Fonts {
    ImFont *body = nullptr;   // base size, regular
    ImFont *strong = nullptr; // base size, semibold (tabs, buttons, emphasis)
    ImFont *small = nullptr;  // 0.82 of the base size (details, footers)
    ImFont *title = nullptr;  // 1.9 of the base size, semibold (page titles)
    float base = 0.0f;        // the base size in pixels
    bool system =
        false; // the system font loaded; false: ImGui's fallback font without the minus, times and ellipsis glyphs
};

// The platform's UI font: Segoe UI and Segoe UI Semibold on Windows.
FontFiles system_font_files();

// The base size for a drawable height and the display's DPI (96 when
// unknown or 0): a 42nd of the height, at least 15 px at 96 DPI scaled by
// the display, at most a 20th of the height, never below 10, floored.
float base_size(int drawable_height, float dpi);

// Owns the ImGui context (keyboard and gamepad navigation on, no .ini or
// log file), the theme and the fonts. Creating one makes it current; create
// the ImGui backends after it and shut them down before it is destroyed.
class Context {
public:
    explicit Context(const Theme &theme = Theme(), FontFiles files = system_font_files());
    ~Context();
    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;

    // Builds the fonts and the style for a base size, between frames. Returns
    // true when they were rebuilt: the renderer backend must then recreate its
    // font texture (ImGui_ImplSDLRenderer2_DestroyFontsTexture, for example).
    bool fit(float base);
    // Makes this context and its ImGui context current.
    void make_current();
    // Turns gamepad navigation on or off (a "controller navigation" setting).
    void set_gamepad_navigation(bool on);

    // The ImGui context this owns.
    ImGuiContext *imgui() const
    {
        return imgui_;
    }

    // The theme in use.
    const Theme &theme() const
    {
        return theme_;
    }

    // The fonts for the last fit.
    const Fonts &fonts() const
    {
        return fonts_;
    }

private:
    ImGuiContext *imgui_ = nullptr;
    Theme theme_;
    FontFiles files_;
    Fonts fonts_;
};

// The current Context; widgets draw with its theme and fonts.
Context &current();

// The UTF-8 text with the minus, times, ellipsis and dash characters
// replaced by ASCII when the current fonts lack them.
std::string glyphs(const char *utf8);

// The usual text of the tag beside a setting that applies at the next launch.
constexpr const char *restart_to_apply = "Restart to apply";

// A dimmed title with a rule after it, opening a group of settings.
void section(const char *title);
// Accent text on the same line as the previous item; nothing when text is null.
void tag(const char *text);
// help, on the setting widgets below, is what the setting does: a dimmed
// "(?)" after the label, and a tooltip with the text, wrapped, while the mouse
// is over the marker or the control (also when it is disabled) or while
// keyboard or gamepad navigation is on the control. Null draws nothing.

// A checkbox. Returns true when it changed.
bool toggle(const char *label, bool *value, const char *help = nullptr);
// A label on the left and a combo on the right half. Returns true when it changed.
bool combo(const char *label, int *value, const char *const *names, int count, const char *note = nullptr,
           const char *help = nullptr);
// A label above a slider with - and + buttons (repeating while held), clamped
// to lo..hi. format is the slider's printf format. Returns true when it changed.
bool slider(const char *label, int *value, int lo, int hi, int step, const char *format, const char *note = nullptr,
            const char *help = nullptr);
// slider with the "%d%%" format.
bool slider_percent(const char *label, int *value, int lo, int hi, int step, const char *note = nullptr,
                    const char *help = nullptr);
// A full-width button. Returns true when pressed.
bool button(const char *label);
// A tab or category button, accent-filled while chosen. Returns true when
// pressed, or when keyboard or gamepad navigation moved onto it.
bool pick_button(const char *label, bool chosen, ImVec2 size);

// The icons the bar's square buttons draw.
enum class Icon {
    reset,
    quit,
    resume
};

// A square button of the given size with an icon drawn in the text colour;
// danger uses the theme's danger colours. Returns true when pressed.
bool icon_button(const char *id, Icon icon, float size, bool danger = false);
// A centred modal question with a confirming button and Cancel (also Escape
// and B). Opens when *ask is set, which it clears. Returns true when confirmed.
bool confirm(const char *id, bool *ask, const char *question, const char *yes, const char *cancel = "Cancel");

// The colour of a status line: none (dimmed), ok or error. Used by note()
// and the first-run screen.
enum class Status {
    none,
    ok,
    error
};

// A row of a reorderable list (a mod, a pack): checkbox, name, details.
struct ListRow {
    std::string id;      // stable identity: the row's ImGui id and what the game saves
    std::string name;    // main text
    std::string detail;  // dimmed text after the name on the same line ("1.2.0 - Author")
    std::string error;   // non-empty: a second line in the theme's error colour; the checkbox is disabled and drawn off
    bool enabled = true; // the checkbox
};

// What the player did to a reorderable list in this frame.
struct ListEdit {
    enum Kind {
        none,
        toggled,
        moved
    };

    Kind kind = none;
    int row = -1; // toggled: the row; moved: its index before the move
    int to = -1;  // moved: its index after the move
};

// How a reorderable list draws.
struct ListOptions {
    float visible_rows = 0.0f;    // height of the scrolling region in rows; 0 = tall enough for every row, no scrolling
    bool checkboxes = true;       // draw the enable checkbox
    bool drag = true;             // the mouse can drag a row to a new place
    const char *empty = nullptr;  // dimmed text shown when there are no rows
    const char *header = nullptr; // dimmed caption above the rows ("Enabled mods"), null = none
};

// A list whose rows the player reorders with the up and down buttons of each
// row, the keyboard, the gamepad or (optionally) the mouse, and switches with
// a checkbox. Moves and toggles are applied to rows in place; the return value
// says which one happened (at most one per frame).
//
// Ctrl or Alt with Up or Down moves the row holding the keyboard focus. A, Enter
// or Space on a row's name picks it up: Up and Down (arrows, D-pad, left stick)
// then move it, and A, Enter, Space, B or Escape put it down. A picked-up row is
// ImGui's active item, so Menu::busy() is true and Escape and B stay with ImGui.
ListEdit reorder_list(const char *id, std::vector<ListRow> &rows, const ListOptions &options = ListOptions());

// A paragraph wrapped to the page width in the dimmed text colour, or in the
// ok or error colour for Status::ok / Status::error.
void note(const char *text, Status status = Status::none);

// A button as wide as its label (button() is full width), so several can share a line with ImGui::SameLine().
bool inline_button(const char *label);

// The device a binding column captures from.
enum class Device {
    keyboard,
    gamepad
};

// An input event translated by the game from its platform's event. code is
// the game's own number (key scancode, button or axis index); the flags say
// what the event means to the menu.
struct InputEvent {
    enum Type {
        other,
        key_down,
        key_up,
        button_down,
        button_up,
        axis,
        mouse_down
    };

    Type type = other;
    int code = 0;        // key, button or axis
    int value = 0;       // axis position, -32768..32767
    bool repeat = false; // a key's auto-repeat
    bool cancel = false; // Escape
    bool clear = false;  // Delete or Backspace
    bool back = false;   // the gamepad's back face button (B)
    bool menu = false;   // the game's menu shortcut (key or button)
};

// What Menu::event did with an event: pass it to ImGui, drop it, or close the menu.
enum class EventResult {
    pass,
    swallow,
    close
};

// One device column of a binding table.
struct BindingColumn {
    const char *title;  // header, "Keyboard"
    Device device;      // what a capture in this column accepts
    const char *prompt; // shown in the cell while capturing, "Press a key"
};

// Rows of actions by device columns, all names and values from the game.
struct BindingTable {
    std::vector<BindingColumn> columns;
    int rows = 0;
    // The action's name.
    std::function<const char *(int row)> label;
    // The binding's text into out; false when unbound (drawn dimmed).
    std::function<bool(int row, int column, char *out, size_t cap)> text;
    // Binds the captured event (null: clear). Returns null, or why it was refused.
    std::function<const char *(int row, int column, const InputEvent *event)> assign;
    // A full-width button under the table (null: none) and what it does.
    const char *restore_label = nullptr;
    std::function<void()> restore;
};

// A sidebar entry and the page it draws.
struct Category {
    const char *name;
    std::function<void()> draw;
};

// A tab of the bar: its categories, or with none, one wide page drawn by draw.
struct Tab {
    const char *name;
    std::vector<Category> categories;
    std::function<void()> draw;
};

// A square button at the bar's right. With a question it asks first.
struct BarButton {
    const char *id; // ImGui id, "##quit"; its confirmation is id + "-confirm"
    Icon icon;
    bool danger = false;
    const char *question = nullptr; // "Quit the game?"
    const char *confirm = nullptr;  // the confirming button's label, "Quit"
};

// The menu shell: tabs, bar buttons, sidebar, page, confirmations, the
// close rules and the binding capture. Make one each time the menu opens.
class Menu {
public:
    Menu(std::vector<Tab> tabs, std::vector<BarButton> buttons);

    // Draws a frame of the menu over a backdrop of opacity_percent (0..100).
    // Returns the index of the bar button pressed (after its confirmation), or -1.
    int draw(int opacity_percent);
    // Applies the close rules and the capture to an event, before ImGui sees it.
    EventResult event(const InputEvent &event);

    // Chooses a tab and its category.
    void select(int tab, int category);
    // Opens the confirmation of a bar button (-1: none).
    void ask(int button);
    // Puts the navigation cursor on the page's first item at the next draw.
    void focus_page();
    // Starts capturing a binding of table, as if its cell had been pressed.
    void begin_capture(const BindingTable *table, int row, int column);
    // Stops a capture without binding.
    void end_capture();
    // Shows text in red under the binding table for four seconds (null: clears).
    void notice(const char *text);

    // The chosen tab.
    int tab() const
    {
        return tab_;
    }

    // The chosen tab's chosen category.
    int category() const
    {
        return category_[tab_];
    }

    // A popup was open or an item active at the last draw (Escape and B are ImGui's).
    bool busy() const
    {
        return busy_;
    }

    // A binding is being captured.
    bool capturing() const
    {
        return capture_row_ >= 0;
    }

private:
    friend void binding_table(Menu &menu, const BindingTable &table);

    std::vector<Tab> tabs_;
    std::vector<BarButton> buttons_;
    std::vector<int> category_;
    std::vector<char> ask_;
    int tab_ = 0;
    const BindingTable *capture_table_ = nullptr;
    int capture_row_ = -1;
    int capture_column_ = 0;
    bool hold_ = false;
    bool busy_ = false;
    bool focus_page_ = false;
    std::string notice_;
    double notice_until_ = 0.0;
};

// Draws a binding table on a page of menu; pressing a cell captures it.
void binding_table(Menu &menu, const BindingTable &table);

// How the game makes textures for the library (the logo): create gets
// straight-alpha 0xAARRGGBB pixels, row by row, and returns a linearly
// filtered, alpha-blended texture of its renderer (0 on failure).
struct TextureCallbacks {
    void *user = nullptr;
    ImTextureID (*create)(void *user, const uint32_t *argb, int width, int height) = nullptr;
    void (*destroy)(void *user, ImTextureID texture) = nullptr;
};

// A one-colour logo from a PNG: its coverage, drawn in its own colour (the
// last fully opaque pixel's), area-averaged to the height drawn.
class Logo {
public:
    Logo(const unsigned char *png, size_t size, TextureCallbacks textures);
    ~Logo();
    Logo(const Logo &) = delete;
    Logo &operator=(const Logo &) = delete;

    // The texture at a height (at most the PNG's), made when the height changes; 0 when none.
    ImTextureID at(int height);
    // Destroys the texture; call before the renderer goes away.
    void release();

    // The size of the last texture.
    int width() const
    {
        return w_;
    }

    int height() const
    {
        return h_;
    }

private:
    TextureCallbacks textures_;
    std::vector<unsigned char> alpha_;
    uint32_t rgb_ = 0;
    int sw_ = 0, sh_ = 0;
    ImTextureID tex_ = 0;
    int w_ = 0, h_ = 0;
};

// A row of the About page's credits.
struct Credit {
    const char *name;
    const char *by;
    const char *licence;
};

// The About page's content; null fields are left out.
struct About {
    const char *title = nullptr;      // the game's name
    const char *version = nullptr;    // shown as "<version_label> <version>"
    const char *build_date = nullptr; // shown as "<built_label> <build_date>"
    const char *developer = nullptr;  // "Developed by Formae"
    Logo *logo = nullptr;             // under the developer
    std::vector<Credit> credits;
    const char *disclaimer = nullptr; // under the credits
    const char *version_label = "Version";
    const char *built_label = "Built";
    const char *credits_title = "Credits";
};

// Draws the About page: identity and credits side by side, or stacked when
// the page is narrower than 40 base sizes.
void about_page(const About &about);

// The first-run screen's texts and state, all from the game.
struct FirstRunContent {
    const char *app_title = nullptr; // in the top bar
    const char *title = nullptr;     // "Choose your ROM"
    const char *text = nullptr;      // the explanation under it
    const char *primary = nullptr;   // the accent button
    const char *secondary = nullptr; // the other button ("Quit")
    bool busy = false;               // buttons disabled
    Status status = Status::none;
    const char *status_text = nullptr; // success or error line
    const char *detail = nullptr;      // smaller text under an error
    const char *footer = nullptr;      // at the bottom of the column
};

// What the player did on the first-run screen this frame.
enum class FirstRunAction {
    none,
    primary,
    secondary
};

// The first-run screen; keep one for the screen's lifetime.
class FirstRunScreen {
public:
    // Draws a frame. Escape and B count as the secondary action.
    FirstRunAction draw(const FirstRunContent &content);

private:
    bool first_frame_ = true;
};

} // namespace formae::ui
