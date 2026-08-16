.POSIX:
.SUFFIXES:

include config.mk

# flags for compiling
DWLCPPFLAGS = -I. -DWLR_USE_UNSTABLE -D_POSIX_C_SOURCE=200809L \
	-DVERSION=\"$(VERSION)\" $(XWAYLAND)
DWLDEVCFLAGS = -g -Wpedantic -Wall -Wextra -Wdeclaration-after-statement \
	-Wno-unused-parameter -Wshadow -Wunused-macros -Werror=strict-prototypes \
	-Werror=implicit -Werror=return-type -Werror=incompatible-pointer-types \
	-Wfloat-conversion

# CFLAGS / LDFLAGS
PKGS      = wayland-server xkbcommon libinput cairo pangocairo $(XLIBS)
DWLCFLAGS = `$(PKG_CONFIG) --cflags $(PKGS)` $(WLR_INCS) $(DWLCPPFLAGS) $(DWLDEVCFLAGS) $(CFLAGS)
LDLIBS    = `$(PKG_CONFIG) --libs $(PKGS)` $(WLR_LIBS) -lm $(LIBS)

all: elmirawm
elmirawm: dwl.o util.o toml.o config_loader.o render_titlebar.o render_menu.o
	$(CC) dwl.o util.o toml.o config_loader.o render_titlebar.o render_menu.o $(DWLCFLAGS) $(LDFLAGS) $(LDLIBS) -o $@
dwl.o: dwl.c client.h config.h config.mk cursor-shape-v1-protocol.h \
	pointer-constraints-unstable-v1-protocol.h wlr-layer-shell-unstable-v1-protocol.h \
	wlr-output-power-management-unstable-v1-protocol.h xdg-shell-protocol.h
util.o: util.c util.h
toml.o: toml.c toml.h
config_loader.o: config_loader.c config_loader.h toml.h
render_titlebar.o: render_titlebar.c render_titlebar.h config_loader.h
render_menu.o: render_menu.c render_menu.h config_loader.h

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
WAYLAND_SCANNER   = `$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner`
WAYLAND_PROTOCOLS = `$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols`

cursor-shape-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/staging/cursor-shape/cursor-shape-v1.xml $@
pointer-constraints-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml $@
wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		protocols/wlr-layer-shell-unstable-v1.xml $@
wlr-output-power-management-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/wlr-output-power-management-unstable-v1.xml $@
xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

config.h:
	cp config.def.h $@
clean:
	rm -f elmirawm dwl *.o *-protocol.h config.h

dist: clean
	mkdir -p elmirawm-$(VERSION)
	cp -R LICENSE* Makefile CHANGELOG.md README.md client.h config.def.h \
		config.mk protocols elmirawm.1 dwl.c util.c util.h elmirawm.desktop \
		elmirawm-$(VERSION)
	tar -caf elmirawm-$(VERSION).tar.gz elmirawm-$(VERSION)
	rm -rf elmirawm-$(VERSION)

install: elmirawm
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	rm -f $(DESTDIR)$(PREFIX)/bin/elmirawm
	cp -f elmirawm $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/elmirawm
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp -f elmirawm.1 $(DESTDIR)$(MANDIR)/man1
	chmod 644 $(DESTDIR)$(MANDIR)/man1/elmirawm.1
	mkdir -p $(DESTDIR)$(DATADIR)/wayland-sessions
	cp -f elmirawm.desktop $(DESTDIR)$(DATADIR)/wayland-sessions/elmirawm.desktop
	chmod 644 $(DESTDIR)$(DATADIR)/wayland-sessions/elmirawm.desktop
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/elmirawm $(DESTDIR)$(MANDIR)/man1/elmirawm.1 \
		$(DESTDIR)$(DATADIR)/wayland-sessions/elmirawm.desktop

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CPPFLAGS) $(DWLCFLAGS) -o $@ -c $<
