# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

SuperSlicer is a FFF/SLA slicer forked from PrusaSlicer (C++17, wxWidgets GUI, OpenGL 3D view).
This repository is the fork `danijel1124/SuperSlicer` of upstream `supermerill/SuperSlicer`.
Its `accessibility` branch holds screen-reader accessibility changes (Windows, NVDA): verify UI
changes through keyboard behaviour and the UI Automation tree, not through visual layout.

## Branches and remotes

- In a clone of this fork, `origin` is `danijel1124/SuperSlicer`. Add upstream once with
  `git remote add upstream https://github.com/supermerill/SuperSlicer.git`.
- Upstream: `dev_27_6x` are development branches (newest `dev_27_64`), `master_27` is older,
  releases are tagged (e.g. `2.7.62.0-beta2`). Windows release/nightly builds come from the
  `nightly_*` branches (see triggers in `.github/workflows/ccpp_win.yml`).
- `dev_27_64` (as of 2026-09-04) on Windows: the `src/test-utils` tools `convert_config` and
  `stl_to_cpp` do not compile, and the GUI crashes during startup (access violation) shortly
  after logging `No <option> in ConfigOptionsGroup config, tab print.ui` for plugin options.
- `CLAUDE.md` and `.claude/` are fork-only files and must not be part of upstream pull requests.

## Building (Windows, MSVC)

Prerequisites: Visual Studio 2022 Build Tools with the C++ workload (`VCTools`, recommended
components incl. Windows SDK) **plus the ATL component** (`Microsoft.VisualStudio.Component.VC.ATL`,
needed by `RemovableDriveManager.cpp`), CMake, Git, Strawberry Perl, gettext (`msgfmt`, for
translations). Use a path without spaces or non-ASCII characters. `resources/profiles` is a
git submodule (`--recurse-submodules`).

CMake 4.x removed compatibility with `cmake_minimum_required` < 3.5, which several dependencies
still use: set `CMAKE_POLICY_VERSION_MINIMUM=3.5` in the environment for both builds below.
The Visual Studio generator finds the toolchain itself; no Native Tools prompt is needed.

Dependencies (once; Release only; output in `deps\build\destdir\usr\local`, downloads in
`deps\build\downloads`; keep the intermediate files in `deps\build`, a rebuild without them
starts from scratch):

```
cd deps && mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DDEP_DEBUG=OFF
cmake --build . --config Release -- /m /nodeReuse:false
```

Application. With Release-only dependencies the configuration list must not contain `Debug`:
`cmake/modules/FindOpenVDB.cmake` otherwise also requires the debug library and reports OpenVDB
as not found. `CMAKE_PREFIX_PATH` must be absolute; set `CMAKE_INSTALL_PREFIX` explicitly or
`INSTALL` targets `C:\Program Files`.

```
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 "-DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo;Release" "-DCMAKE_PREFIX_PATH=<repo>\deps\build\destdir\usr\local" "-DCMAKE_INSTALL_PREFIX=<repo>\build\install"
cmake --build . --config RelWithDebInfo --target ALL_BUILD -- /m /nodeReuse:false
cmake --build . --config RelWithDebInfo --target gettext_po_to_mo -- /m /nodeReuse:false
cmake --build . --config RelWithDebInfo --target slic3rDllsCopy -- /m /nodeReuse:false
```

`/nodeReuse:false` keeps idle `MSBuild.exe` nodes from lingering (they block the VS installer).
The runnable program is in `build\src\RelWithDebInfo\` (all DLLs and a copied
`resources` folder next to it); `build\install` lacks DLLs. `gettext_po_to_mo` writes the `.mo`
files into the source tree `resources\localization\*\` only — copy them into
`build\src\RelWithDebInfo\resources\localization\`.

Upstream CI (`.github/workflows/ccpp_win.yml`) and `build_win.bat` use equivalent steps with
`msbuild` but build Debug dependencies as well (no `DEP_DEBUG=OFF`); full guide: `doc/How to build - Windows.md`. Linux: `BuildLinux.sh`.

## Running and debugging

- App name, key and executable come from `version.inc` (`SLIC3R_APP_KEY`, `SLIC3R_APP_CMD`):
  `SuperSlicer` / `superslicer.exe` in 2.7.62, `Slic3r` / `Slic3r.exe` in `dev_27_64`. The
  configuration lives in `%APPDATA%\<AppKey>\<AppKey>_<version>\`; `%APPDATA%\<AppKey>\installed.ini`
  lists all installations and their configuration folders.
- First start of a new version or location shows a "First launch" dialog (`choose_app_dir` in
  `GUI_App.cpp`): "Yes" copies an existing configuration into a new folder; "Let me choose" also
  offers "Use same configuration as ..." for an installation of the same version, which shares
  that installation's live folder instead of copying it.
- `--datadir <dir>` uses an isolated configuration directory and skips that dialog.
- `<app>_console.exe --loglevel 5 [--datadir <dir>]` (`superslicer_console.exe` in 2.7.62)
  starts the GUI with log output on the console; `<app>_console.exe --help` lists the CLI and
  exercises startup without the GUI.

## Tests

Unit tests use Catch2 and are off by default: configure with `-DSLIC3R_BUILD_TESTS=ON`, build,
then run `ctest` in the build directory (or a single test executable under `build/tests/...`).
Test sources are in `tests/` (`fff_print`, `libslic3r`, `sla_print`, `superslicerlibslic3r`, ...).

## Architecture

- `src/libslic3r/` — slicing core, no GUI dependency. Config system: `PrintConfig.cpp` (all
  option definitions and enums), `Preset.cpp` / `PresetBundle.cpp` (print/filament/printer
  presets and physical printers), `AppConfig.cpp` (application preferences in `<AppKey>.ini`,
  defaults in `AppConfig::set_defaults`). In 2.7.62 / `master_27` these files are directly in
  `src/libslic3r/`; in `dev_27_64` they are in `src/libslic3r/Config/`.
- `src/slic3r/GUI/` — wxWidgets application: `MainFrame`, `Plater` (sidebar with preset combos),
  `Tab` (settings tabs), `Preferences` (preferences dialog), `PresetComboBoxes` (printer/print/
  filament selectors, including separator and "Add/Remove ..." wizard items), `Field.cpp`
  (option controls on settings tabs; enum options use native `wxBitmapComboBox`).
- `src/slic3r/GUI/Widgets/` — custom-drawn widgets (`ComboBox`, `DropDown`, `TextInput`, ...).
  These are not native controls: in UI Automation they appear as an unnamed-role "Pane", and
  their keyboard handling is implemented manually (e.g. `ComboBox::keyDown`).
- Accessible view (`accessibility` branch): app config key `accessible_view`, toggled by
  View > Accessible view (`MainFrame.cpp`, rebuilds the GUI). `ComboBox::UseNativeControl`
  covers a custom combo box with a native `wxChoice` that mirrors items, selection and events and
  exposes an accessible name; `PresetComboBox` enables it and skips separators on arrow keys.
- `src/slic3r/GUI/BitmapComboBox.*` — preset combo base class, derived from `::ComboBox`
  (custom widget); the former native `wxBitmapComboBox` base is still present as commented code.
- `src/slic3r/Utils/` — print host uploads (`PrintHost.cpp` factory, `Moonraker`, `OctoPrint`,
  `Klipper` = OctoPrint API variant), network and update utilities.
- `resources/` — icons, localization (`.po` sources), vendor profiles (`resources/profiles`
  submodule) and `ui_layout/default/*.ui`, text files that define which options appear on which
  settings page; the GUI builds the settings tabs from them at startup.
- Plugins (in `dev_27_64`, not in 2.7.62 / `master_27`): built-in C++ plugins in
  `src/plugins_cpp/`, Python plugins in `src/plugins_python/`, design docs in `doc/plugins/`. They are packaged at build time and
  unpacked into the configuration directory (`plugins/`, `cache/plugins/`) at runtime; plugin
  options are referenced from the `.ui` layouts.

## Conventions

- Formatting: `.clang-format` in the repository root.
