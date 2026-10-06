# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Project

SuperSlicer is a FFF/SLA slicer forked from PrusaSlicer (C++17, wxWidgets GUI, OpenGL 3D view).
This fork (`danijel1124/SuperSlicer`, upstream `supermerill/SuperSlicer`) adds screen-reader
accessibility fixes. The maintainer of this fork is blind and uses NVDA on Windows: verify UI
changes through keyboard behaviour and the UI Automation tree, never through visual layout.

The active work plan lives in `.claude/rules/accessibility-plan.md`.

## Branches and remotes

- `accessibility` — working branch of this fork, based on upstream `dev_27_64`.
- Upstream development branches are `dev_27_6x` (newest: `dev_27_64`); `master_27` is older.
- In a clone of this fork, `origin` is `danijel1124/SuperSlicer`. Add upstream once with
  `git remote add upstream https://github.com/supermerill/SuperSlicer.git`.
- `CLAUDE.md` and `.claude/` are fork-only files and must not be part of upstream pull requests.

## Building (Windows, MSVC)

Builds are done locally (Windows VM), not in cloud CI.

Prerequisites: Visual Studio 2022 with the C++ desktop workload, CMake, Git, Strawberry Perl.
Clone into a short path without spaces or non-ASCII characters, with submodules
(`resources/profiles` is a submodule):

```
git clone --recurse-submodules -b accessibility https://github.com/danijel1124/SuperSlicer.git C:\src\SuperSlicer
```

Dependencies (built once, slow; run in an "x64 Native Tools Command Prompt for VS 2022"):

```
cd C:\src\SuperSlicer\deps
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DDEP_DEBUG=OFF
msbuild /m ALL_BUILD.vcxproj
```

Application (`CMAKE_PREFIX_PATH` must be absolute):

```
cd C:\src\SuperSlicer
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\src\SuperSlicer\deps\build\destdir\usr\local"
msbuild /m /P:Configuration=RelWithDebInfo INSTALL.vcxproj
msbuild /m /P:Configuration=RelWithDebInfo gettext_po_to_mo.vcxproj
msbuild /m /P:Configuration=RelWithDebInfo src\occt_wrapper\OCCTWrapper.vcxproj
msbuild /m /P:Configuration=RelWithDebInfo src\slic3rDllsCopy.vcxproj
```

The sequence mirrors `.github/workflows/ccpp_win.yml`. Alternative all-in-one script:
`build_win.bat -d=..\SuperSlicer-deps -r=none` (`build_win.bat -?` lists options;
`-s=app-dirty` does an incremental app build). Full guide: `doc/How to build - Windows.md`.

Linux: `BuildLinux.sh` (see `doc/How to build - Linux et al.md`).

## Tests

Unit tests use Catch2 and are off by default: configure with `-DSLIC3R_BUILD_TESTS=ON`, build,
then run `ctest` in the build directory (or a single test executable under `build/tests/...`).
Test sources are in `tests/` (`fff_print`, `libslic3r`, `sla_print`, `superslicerlibslic3r`, ...).

## Architecture

- `src/libslic3r/` — slicing core, no GUI dependency. `Config/` holds the config system:
  `PrintConfig.cpp` (all option definitions and enums), `Preset.cpp` / `PresetBundle.cpp`
  (print/filament/printer presets and physical printers), `AppConfig.cpp` (application
  preferences in `SuperSlicer.ini`, defaults in `AppConfig::set_defaults`).
- `src/slic3r/GUI/` — wxWidgets application: `MainFrame`, `Plater` (sidebar with preset combos),
  `Tab` (settings tabs), `Preferences` (preferences dialog), `PresetComboBoxes` (printer/print/
  filament selectors, including separator and "Add/Remove ..." wizard items), `Field.cpp`
  (option controls on settings tabs; enum options use native `wxBitmapComboBox`).
- `src/slic3r/GUI/Widgets/` — custom-drawn widgets (`ComboBox`, `DropDown`, `TextInput`, ...).
  These are not native controls: in UI Automation they appear as an unnamed-role "Pane", and
  their keyboard handling is implemented manually (e.g. `ComboBox::keyDown`).
- `src/slic3r/GUI/BitmapComboBox.*` — preset combo base class, derived from `::ComboBox`
  (custom widget); the former native `wxBitmapComboBox` base is still present as commented code.
- `src/slic3r/Utils/` — print host uploads (`PrintHost.cpp` factory, `Moonraker`, `OctoPrint`,
  `Klipper` = OctoPrint API variant), network and update utilities.
- `resources/` — icons, localization, vendor profiles (`resources/profiles` submodule).
- User configuration on Windows: `%APPDATA%\SuperSlicer\<version folder>\` (`SuperSlicer.ini`,
  `printer/`, `print/`, `filament/`, `physical_printer/`).

## Conventions

- Code, comments and commit messages in English.
- Comments describe current behaviour only; history and rationale belong in commit messages.
- Keep upstream-facing changes small and focused so they can be offered as pull requests.
