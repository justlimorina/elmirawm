#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <stdbool.h>

typedef struct {
	char *label;
	char *cmd;
	char *action; /* "reload", "quit", "separator", or NULL */
} MenuItem;

typedef struct {
	char *terminal;
	char *launcher;

	/* Theme & Visuals */
	int border_radius;
	int border_width;

	float active_border_color[4];
	float inactive_border_color[4];

	/* Menu */
	MenuItem *menu_items;
	int menu_item_count;

	/* Autostart */
	char **autostart_cmds;
	int autostart_count;

	/* Shell integration */
	char *shell_cmd;
	int shell_enable;
} ElmiraConfig;

extern ElmiraConfig g_config;

void load_config(void);
void free_config(void);

#endif /* CONFIG_LOADER_H */
