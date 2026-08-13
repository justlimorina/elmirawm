# elmirawm version
_VERSION = 0.8-dev
VERSION  = `git describe --tags --dirty 2>/dev/null || echo $(_VERSION)`

# Build tool definitions
PKG_CONFIG = pkg-config
CC         = cc

# Installation paths
PREFIX  = /usr/local
MANDIR  = $(PREFIX)/share/man
DATADIR = $(PREFIX)/share

# Compiler & Linker flags
CFLAGS  ?= -O2 -pipe
LDFLAGS ?= -Wl,-O1,--as-needed

# Dynamic wlroots headers and libraries detection (supports wlroots-0.20, wlroots-0.19, wlroots)
WLR_INCS = `$(PKG_CONFIG) --cflags wlroots-0.20 2>/dev/null || $(PKG_CONFIG) --cflags wlroots-0.19 2>/dev/null || $(PKG_CONFIG) --cflags wlroots`
WLR_LIBS = `$(PKG_CONFIG) --libs wlroots-0.20 2>/dev/null || $(PKG_CONFIG) --libs wlroots-0.19 2>/dev/null || $(PKG_CONFIG) --libs wlroots`

# XWayland Support (Enabled for X11 app compatibility like Chrome, GTK X11, legacy apps)
XWAYLAND = -DXWAYLAND
XLIBS    = xcb xcb-icccm
