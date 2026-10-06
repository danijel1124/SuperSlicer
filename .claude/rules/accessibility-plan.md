# Accessibility plan (fork-only file)

## Goal

Make the preset selectors (printer, print, filament, SLA material) usable with the keyboard and
the NVDA screen reader on Windows.

## Problem

- Preset selectors use the custom widget `src/slic3r/GUI/Widgets/ComboBox.cpp` (via
  `BitmapComboBox`). In UI Automation it is an unnamed-role "Pane" with no actions; NVDA only
  reads its current label.
- `ComboBox::keyDown`: with the list closed, Up/Down select the neighbouring item immediately.
  Separator items (`LABEL_ITEM_MARKER`) make `PlaterPresetComboBox::OnSelect` /
  `TabPresetComboBox::OnSelect` (`PresetComboBoxes.cpp`) jump back to the previous item, and the
  trailing "Add/Remove printers" item (`LABEL_ITEM_WIZARD_PRINTERS`, also filaments/materials)
  opens the configuration wizard. An item enclosed by a separator and the wizard item (e.g. a
  single physical printer) cannot be left with the arrow keys.
- Working behaviour today: Enter opens the list; Up/Down and first-letter keys then only move the
  highlight; Enter commits. This is undiscoverable for screen-reader users.
- The same arrow-key trap exists with native combo boxes, so it must be fixed in either mode.

## Decisions

- Accessible view: app config key `accessible_view` (default off), check item View > Accessible
  view; switching saves the key and rebuilds the GUI (`recreate_GUI`). The normal view stays
  unchanged so its behaviour matches other slicers.
- The accessible view uses only standard controls. The custom widgets switch themselves
  (least invasive): `ComboBox::UseNativeControl` covers the widget with a native control that
  mirrors items/selection/events and exposes an accessible name.
- Combo boxes: read-only lists become `wxChoice`, editable ones `wxComboBox`; no
  `wxBitmapComboBox` (owner-drawn). Icon meanings (e.g. incompatible, system preset) become a text
  suffix like the existing "(modified)".
- Separators and the wizard item stay in the lists. Arrow keys skip separators in the direction of
  travel (on the settings tabs also disabled items); the wizard item behaves as today. Standard
  combo box behaviour applies: closed list selects immediately (unsaved-changes dialog right
  away), open list (Alt+Down) commits only with Enter.
- Before switching the view, ask to save a modified project and modified presets (as on exit).
- Accessible names: the visible label where one exists; otherwise the parameter key (the
  "parameter name" at the end of the tooltip, `Field::get_tooltip_text`).

Rejected: a help text only (does not remove the arrow-key trap or give the control a role).

## Phases

1. All combo boxes: native control enabled in the `ComboBox` constructor when `accessible_view`
   is on (preset combos done as prototype), accessible names, icon meanings as text, save prompt
   before switching.
2. Check boxes, spin inputs, text inputs.
3. Names for unlabeled native buttons (e.g. the cog button next to the printer combo).

## Code pointers (branch `accessibility`, based on upstream tag `2.7.62.0-beta2`)

- `src/slic3r/GUI/Widgets/ComboBox.cpp` — `UseNativeControl`, `ComboBox::keyDown`, `sendComboBoxEvent`.
- `src/slic3r/GUI/Field.cpp` — option controls on the settings tabs (`Choice::BUILD`, tooltip with
  "parameter name" in `get_tooltip_text`).
- `src/slic3r/GUI/MainFrame.cpp` — View menu (`init_menubar_as_editor`).
- `src/slic3r/GUI/Widgets/DropDown.cpp` — popup list and its key forwarding.
- `src/slic3r/GUI/BitmapComboBox.{hpp,cpp}` — preset combo base; native base is commented out.
- `src/slic3r/GUI/PresetComboBoxes.cpp` — list building (`update()`), label markers, `OnSelect`.
- `src/slic3r/GUI/Preferences.cpp` — preference checkboxes (pattern: `use_legacy_3DConnexion`).
- `src/libslic3r/AppConfig.cpp` — preference defaults in `set_defaults`.

## Build and test

- Builds run locally only (local Windows build machine, MSVC 2022), never in cloud CI. Build steps:
  `CLAUDE.md`.
- Verify with NVDA and the UI Automation tree: role, name/value announcement, arrow keys in
  closed and open state, Enter, first-letter keys, separators skipped.
- Prototype tested (sidebar printer list): combo box role and name, selection of printers and
  physical printers, separators skipped closed / read open, wizard via arrow key. Still to test:
  focus after a preset change on a settings tab, physical printer dialog, unsaved-changes dialog
  (closed and open list), switching back, start with the key already set.

## Upstream

- Pull request against the current upstream development branch (`dev_27_6x`): port the code
  changes onto it in a separate branch that contains only those changes (no `CLAUDE.md`, no
  `.claude/`).
- Issue on `supermerill/SuperSlicer` describing the problem: drafted in English, submitted only
  after the user approved the final text. Mention the alternative (replacing the custom widgets by
  native control classes instead of switching inside the widgets).

## Open

- Whether the full tooltip is also exposed as accessible description.
