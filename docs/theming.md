# Theming

## Colours

All colours live in `formae::ui::Theme`, as ImGui `IM_COL32` values. Copy the default, change what
you need and pass it to the `Context`:

```cpp
formae::ui::Theme theme;
theme.accent = IM_COL32(88, 166, 255, 255);
theme.accent_hi = IM_COL32(128, 188, 255, 255);
theme.on_accent = IM_COL32(8, 20, 36, 255);
formae::ui::Context ui(theme);
```

| Field | Default | Used for |
|---|---|---|
| `accent` | `242, 179, 61` | Chosen tab and category, the rule under the bar, checks, slider grabs, tags, the primary first-run button, navigation cursor |
| `accent_hi` | `252, 200, 102` | Accent elements hovered or held |
| `on_accent` | `26, 19, 6` | Text on accent fills |
| `backdrop` | `11, 13, 18` | Behind the menu (alpha from the opacity) and the first-run screen |
| `text`, `text_dim`, `text_off` | `236, 239, 243` / `150, 158, 171` / `98, 105, 118` | Body, secondary and disabled text |
| `line` | white at 34 | Separators, borders, the sidebar rule |
| `soft_hover`, `soft_active` | white at 22 / 40 | Unchosen tabs and categories under the pointer or held |
| `danger`, `danger_hi` | `172, 56, 50` / `204, 74, 66` | Destructive bar buttons |
| `error_text`, `ok_text` | `239, 112, 100` / `126, 205, 140` | Refusals and success lines |
| `popup` | `24, 28, 36` at 252 | Combo lists and confirmations |
| `frame`, `frame_hovered`, `frame_active` | `40, 46, 58` ... | Slider, combo and checkbox frames |
| `button`, `button_hovered`, `button_active` | `44, 51, 64` ... | Buttons |
| `scrollbar`, `scrollbar_hovered`, `scrollbar_active` | white at 40 / 64 / 90 | Scrollbar grab |
| `row_alt` | white at 7 | Alternate rows of the credits |
| `modal_dim` | black at 120 | Behind a confirmation |
| `header_alpha`, `selection_alpha` | 64, 90 | Accent alpha behind a selected combo entry and selected text |

The theme is applied to ImGui's style in `Context::fit`, so plain ImGui widgets drawn on a page
match the rest.

## Fonts

`FontFiles` names a regular and a semibold TrueType file; the default, `system_font_files()`, is
Segoe UI and Segoe UI Semibold on Windows. Pass your own to ship a font with the game:

```cpp
formae::ui::Context ui(formae::ui::Theme(), { "assets/fonts/Inter-Regular.ttf", "assets/fonts/Inter-SemiBold.ttf" });
```

Four fonts are built from them at every base size:

| `Fonts` member | Size | Used for |
|---|---|---|
| `body` | base | Everything by default |
| `strong` | base, semibold | Tabs, the first-run buttons, credits names, confirmation questions |
| `small` | 0.82 base | Details and footers |
| `title` | 1.9 base, semibold | Page titles (About, first run) |

The glyph ranges are ImGui's default plus the en and em dashes, the ellipsis, the minus sign and the
multiplication sign. When the files cannot be loaded, ImGui's built-in font is used and
`formae::ui::glyphs` (applied by the sliders and the first-run buttons) replaces those characters
with ASCII.

## Sizes

Every measure is a multiple of one base size, `formae::ui::base_size(drawable_height, dpi)`: a 42nd
of the drawable height, at least 15 px at 96 DPI (scaled by the display's DPI), at most a 20th of the
height, never below 10. At 720 p that is 17 px, at 1080 p 25 px, at 2160 p 51 px. Some of the
proportions:

| Element | Size |
|---|---|
| Outer margin | 1.1 base |
| Bar height, icon buttons | 2 base |
| Sidebar width | 10 base |
| Page width | at most 36 base beside a sidebar, 60 base alone |
| Category button height | 1.9 base |
| Accent rule | 0.12 base, at least 1 px |

Pass a different height (a fixed 720, for example) to keep the menu the same size whatever the
window.
