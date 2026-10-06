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

1. Keyboard fix (all users): with the list closed, Up/Down skip separator items and never select
   a wizard item; the wizard opens only when its item is explicitly committed from the open list.
2. New preference "Use native combo boxes" (default off): preset selectors become native Windows
   combo boxes (`wxBitmapComboBox`, already used for enum options in `Field.cpp`) so NVDA gets the
   combo box role, value and list. Decision 1 applies in this mode as well.

Rejected: a help text only (does not remove the arrow-key trap or give the control a role).

## Code pointers (branch `accessibility`, based on upstream `dev_27_64`)

- `src/slic3r/GUI/Widgets/ComboBox.cpp` — `ComboBox::keyDown`, `sendComboBoxEvent`.
- `src/slic3r/GUI/Widgets/DropDown.cpp` — popup list and its key forwarding.
- `src/slic3r/GUI/BitmapComboBox.{hpp,cpp}` — preset combo base; native base is commented out.
- `src/slic3r/GUI/PresetComboBoxes.cpp` — list building (`update()`), label markers, `OnSelect`.
- `src/slic3r/GUI/Preferences.cpp` — preference checkboxes (pattern: `use_legacy_3DConnexion`).
- `src/libslic3r/Config/AppConfig.cpp` — preference defaults in `set_defaults`.

## Build and test

- Builds run locally only (local Windows build machine, MSVC 2022), never in cloud CI. Build steps:
  `CLAUDE.md`.
- Verify with NVDA and the UI Automation tree: role, name/value announcement, arrow keys in
  closed and open state, Enter, first-letter keys, no wizard on arrow keys.

## Upstream

- Pull request against `supermerill/SuperSlicer` `dev_27_64` from a separate branch that
  contains only the code changes (no `CLAUDE.md`, no `.claude/`).
- Issue on `supermerill/SuperSlicer` describing the problem: drafted in English, submitted only
  after the user approved the final text.

## Open

- How the preference switches the base control (two combo classes vs. one class wrapping either
  control); decide after reading `BitmapComboBox` and `PresetComboBoxes` in detail.
- Whether the preference applies immediately or after restart.
