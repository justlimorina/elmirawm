#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config_loader.h"
#include "toml.h"

ElmiraConfig g_config;

static void parse_hex_color(const char *hex, float color[4]) {
	unsigned int r = 0, g = 0, b = 0, a = 255;
	if (!hex || strlen(hex) < 7 || hex[0] != '#') {
		return;
	}
	if (strlen(hex) >= 9) {
		sscanf(hex + 1, "%02x%02x%02x%02x", &r, &g, &b, &a);
	} else {
		sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
	}
	color[0] = r / 255.0f;
	color[1] = g / 255.0f;
	color[2] = b / 255.0f;
	color[3] = a / 255.0f;
}

static void add_menu_item(const char *label, const char *cmd, const char *action) {
	g_config.menu_items = realloc(g_config.menu_items, (g_config.menu_item_count + 1) * sizeof(MenuItem));
	g_config.menu_items[g_config.menu_item_count].label = label ? strdup(label) : NULL;
	g_config.menu_items[g_config.menu_item_count].cmd = cmd ? strdup(cmd) : NULL;
	g_config.menu_items[g_config.menu_item_count].action = action ? strdup(action) : NULL;
	g_config.menu_item_count++;
}

static void set_defaults(void) {
	g_config.terminal = NULL; /* fall back to termcmd from config.h */
	g_config.launcher = strdup("fuzzel");
	g_config.border_radius = 16;
	g_config.border_width = 3;

	/* Active Border: Lavender #D0BCFF */
	parse_hex_color("#D0BCFF", g_config.active_border_color);
	/* Inactive Border: Dark Gray #49454F */
	parse_hex_color("#49454F", g_config.inactive_border_color);

	g_config.menu_items = NULL;
	g_config.menu_item_count = 0;

	/* Default Context Menu Items (in English) */
	add_menu_item("Terminal", "ptyxis", NULL);
	add_menu_item("Files", "nautilus", NULL);
	add_menu_item("Browser", "firefox", NULL);
	add_menu_item(NULL, NULL, "separator");
	add_menu_item("Applications", "fuzzel", NULL);
	add_menu_item(NULL, NULL, "separator");
	add_menu_item("Reload Config", NULL, "reload");
	add_menu_item("Exit Elmira", NULL, "quit");

	g_config.autostart_cmds = NULL;
	g_config.autostart_count = 0;

	g_config.shell_cmd = strdup("elmira-shell");
	g_config.shell_enable = 0;
}

void load_config(void) {
	const char *home;
	char path[512];
	FILE *fp;
	char errbuf[200];
	toml_table_t *conf;
	toml_table_t *gen;
	toml_table_t *theme;
	toml_table_t *auto_sec;
	toml_table_t *shell_sec;

	set_defaults();

	home = getenv("HOME");
	if (!home) return;

	snprintf(path, sizeof(path), "%s/.config/elmira/config.toml", home);
	fp = fopen(path, "r");
	if (!fp) {
		snprintf(path, sizeof(path), "%s/.config/elmirawm/config.toml", home);
		fp = fopen(path, "r");
	}
	if (!fp) return;

	conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
	fclose(fp);

	if (!conf) return;

	/* [general] */
	gen = toml_table_in(conf, "general");
	if (gen) {
		char *val = NULL;
		if (toml_string_in(gen, "terminal", &val) == 0 && val) {
			if (g_config.terminal) free(g_config.terminal);
			g_config.terminal = val;
		}
		val = NULL;
		if (toml_string_in(gen, "launcher", &val) == 0 && val) {
			if (g_config.launcher) free(g_config.launcher);
			g_config.launcher = val;
		}
	}

	/* [theme] */
	theme = toml_table_in(conf, "theme");
	if (theme) {
		int64_t ival = 0;
		char *hex = NULL;
		if (toml_int_in(theme, "border_radius", &ival) == 0) g_config.border_radius = (int)ival;
		if (toml_int_in(theme, "border_width", &ival) == 0) g_config.border_width = (int)ival;

		if (toml_string_in(theme, "active_border", &hex) == 0 && hex) {
			parse_hex_color(hex, g_config.active_border_color);
			free(hex);
		}
		if (toml_string_in(theme, "inactive_border", &hex) == 0 && hex) {
			parse_hex_color(hex, g_config.inactive_border_color);
			free(hex);
		}
	}

	/* [autostart] */
	auto_sec = toml_table_in(conf, "autostart");
	if (auto_sec) {
		toml_array_t *arr = toml_array_in(auto_sec, "exec");
		if (arr) {
			int count = toml_array_nelem(arr);
			if (count > 0) {
				int i;
				g_config.autostart_cmds = calloc(count, sizeof(char *));
				for (i = 0; i < count; i++) {
					char *cmd = NULL;
					if (toml_string_at(arr, i, &cmd) == 0 && cmd) {
						g_config.autostart_cmds[g_config.autostart_count++] = cmd;
					}
				}
			}
		}
	}

	/* [shell] */
	shell_sec = toml_table_in(conf, "shell");
	if (shell_sec) {
		char *scmd = NULL;
		int bval = 0;
		if (toml_string_in(shell_sec, "command", &scmd) == 0 && scmd) {
			if (g_config.shell_cmd) free(g_config.shell_cmd);
			g_config.shell_cmd = scmd;
		}
		if (toml_bool_in(shell_sec, "enable", &bval) == 0) {
			g_config.shell_enable = bval;
		}
	}

	/* [[item]] or [[menu.item]] custom menu configuration */
	{
		int item_idx = 0;
		bool custom_menu = false;
		toml_table_t *item_tab;

		while ((item_tab = toml_subtable_at(conf, "item", item_idx)) != NULL ||
		       (item_tab = toml_subtable_at(conf, "menu.item", item_idx)) != NULL) {
			char *lbl = NULL, *cmd = NULL, *act = NULL, *type = NULL;

			if (!custom_menu) {
				int i;
				for (i = 0; i < g_config.menu_item_count; i++) {
					if (g_config.menu_items[i].label) free(g_config.menu_items[i].label);
					if (g_config.menu_items[i].cmd) free(g_config.menu_items[i].cmd);
					if (g_config.menu_items[i].action) free(g_config.menu_items[i].action);
				}
				free(g_config.menu_items);
				g_config.menu_items = NULL;
				g_config.menu_item_count = 0;
				custom_menu = true;
			}

			toml_string_in(item_tab, "label", &lbl);
			toml_string_in(item_tab, "cmd", &cmd);
			toml_string_in(item_tab, "action", &act);
			toml_string_in(item_tab, "type", &type);

			if (type && strcmp(type, "separator") == 0) {
				add_menu_item(NULL, NULL, "separator");
			} else {
				add_menu_item(lbl, cmd, act);
			}

			if (lbl) free(lbl);
			if (cmd) free(cmd);
			if (act) free(act);
			if (type) free(type);

			item_idx++;
		}
	}

	toml_free(conf);
}

void free_config(void) {
	int i;
	if (g_config.terminal) free(g_config.terminal);
	if (g_config.launcher) free(g_config.launcher);
	if (g_config.shell_cmd) free(g_config.shell_cmd);

	if (g_config.menu_items) {
		for (i = 0; i < g_config.menu_item_count; i++) {
			if (g_config.menu_items[i].label) free(g_config.menu_items[i].label);
			if (g_config.menu_items[i].cmd) free(g_config.menu_items[i].cmd);
			if (g_config.menu_items[i].action) free(g_config.menu_items[i].action);
		}
		free(g_config.menu_items);
		g_config.menu_items = NULL;
		g_config.menu_item_count = 0;
	}

	if (g_config.autostart_cmds) {
		for (i = 0; i < g_config.autostart_count; i++) {
			if (g_config.autostart_cmds[i]) free(g_config.autostart_cmds[i]);
		}
		free(g_config.autostart_cmds);
		g_config.autostart_cmds = NULL;
		g_config.autostart_count = 0;
	}
}
