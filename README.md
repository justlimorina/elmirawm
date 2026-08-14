# Elmira (elmirawm)

**Elmira** (`elmirawm`) is a modern, lightweight, **floating-first** Wayland compositor based on [`dwl`](https://codeberg.org/dwl/dwl) and [`wlroots`](https://gitlab.freedesktop.org/wlroots/wlroots). It combines minimalist Wayland compositor architecture with a state-of-the-art **Material Design 3 (MD3)** Server-Side Decoration (SSD) system and dynamic TOML configuration.

![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)
![Wayland](https://img.shields.io/badge/Protocol-Wayland-orange.svg)
![MD3](https://img.shields.io/badge/Design-Material_Design_3-purple.svg)

---

## Features

- **Floating-First Window Management**: Windows spawn floating and intelligently centered on screen by default. Tiling is retained as a secondary fallback layout (`layouts[1]`).
- **Native Material Design 3 Server-Side Decorations (SSD)**:
  - **Dynamic Titlebar**: Rendered with **Cairo** & **Pango** with centered text, auto-ellipsization, and MD3 surface container colors.
  - **Fully Rounded Outer Borders**: Vector-rendered anti-aliased rounded outer window borders (`border_radius` configurable).
  - **MD3 Elevation Drop Shadows**: Real-time ambient gaussian drop shadows (Elevation Level 3 for active window, Elevation Level 1 for inactive window).
- **Desktop Context Menu (MD3)**: Native Cairo/Pango vector-rendered right-click menu featuring MD3 Surface Container High palette (`#2B2930`), 16px outer rounded corners, and 8px hover highlights.
- **Single-Tap `Super` Key Launcher**: Instant application launcher invocation (defaults to `fuzzel`) on a quick `Super` key release.
- **TOML Configuration System (`~/.config/elmirawm/config.toml`)**:
  - Customize terminal emulator, launcher, theme borders, radius, active/inactive colors.
  - Autostart background processes via multiline TOML arrays.
  - Fully customizable right-click context menu items (`[[menu.item]]`).
- **High Performance & Memory Safe**: Custom CPU-backed RAM buffer architecture (`DATA_PTR_ACCESS`) ensuring zero GPU allocator conflicts and ultra-low RAM footprint.

---

## Building & Dependencies

### Prerequisites

Elmira requires the following development packages:

- `wayland-server` & `wayland-protocols`
- `wlroots` (0.20+)
- `libinput` & `xkbcommon`
- `libcairo2-dev` & `libpango1.0-dev` (for MD3 SSD & Menu rendering)
- `pkg-config` & `make`

On **Debian/Ubuntu-based** systems:
```bash
sudo apt update
sudo apt install -y build-essential pkg-config libcairo2-dev libpango1.0-dev \
    libwlroots-dev libwayland-dev wayland-protocols libxkbcommon-dev libinput-dev
```

### Compiling

Clone the repository and compile using `make`:

```bash
git clone https://github.com/justlimorina/elmirawm.git
cd elmirawm
make
```

To install system-wide:
```bash
sudo make install
```

To uninstall:
```bash
sudo make uninstall
```
---

## Running Elmira

Elmira can be run directly from a VT console or nested within an existing X11/Wayland session for testing:

```bash
./elmirawm
```

### Autostart Integration

Autostart commands can be specified in `~/.config/elmirawm/config.toml` or via the `-s` startup script flag:

```bash
./elmirawm -s 'waybar & <&-'
```

---

## Configuration

Configuration is managed via `~/.config/elmirawm/config.toml`. If no configuration file is present, Elmira loads safe built-in defaults.

See [`config.toml.example`](config.toml.example) for a complete reference.

### Example `config.toml`

```toml
[general]
terminal = "ptyxis"
launcher = "fuzzel"

[theme]
border_radius = 16
border_width = 3
active_border = "#D0BCFF"
inactive_border = "#49454F"

[autostart]
exec = [
    "fcitx5 -d",
    # "waybar"
]

# Custom Right-Click Context Menu
[[menu.item]]
label = "Terminal"
cmd = "ptyxis"

[[menu.item]]
label = "Files"
cmd = "nautilus"

[[menu.item]]
label = "Browser"
cmd = "firefox"

[[menu.item]]
type = "separator"

[[menu.item]]
label = "Applications"
cmd = "fuzzel"

[[menu.item]]
type = "separator"

[[menu.item]]
label = "Reload Config"
action = "reload"

[[menu.item]]
label = "Exit Elmira"
action = "quit"
```

---

## Architecture Overview

```
elmirawm/
├── elmira.c            # Core Wayland compositor logic & scene graph management
├── render_titlebar.c   # Cairo/Pango MD3 titlebar, border & shadow rendering
├── render_titlebar.h   # Public interfaces for titlebar/border/shadow generators
├── render_menu.c       # Cairo/Pango MD3 desktop context menu rendering engine
├── render_menu.h       # Public interfaces for context menu rendering & hit testing
├── toml.c              # Lightweight C TOML configuration parser
├── toml.h              # TOML parser API
├── config_loader.c     # Configuration file loader & dynamic theme manager
├── config_loader.h     # ElmiraConfig data structures & default definitions
├── client.h            # Client window wrapper functions & helpers
├── config.def.h        # C fallback constants, layout options, and keybindings
├── config.toml.example # Sample user configuration file
└── Makefile            # Build system with Cairo & Pango integration
```

---

## License & Acknowledgements

Elmira is licensed under the **GNU General Public License v3.0 (GPLv3)**. See [`LICENSE`](LICENSE) for details.

### Credits
- Based on [`dwl`](https://codeberg.org/dwl/dwl) by Devin J. Pohly and the `dwl` contributor community.
- Inspired by [`dwm`](https://dwm.suckless.org/) by `suckless.org`.
- Built upon [`wlroots`](https://gitlab.freedesktop.org/wlroots/wlroots) by the Sway/wlroots team.
