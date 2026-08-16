#ifndef RENDER_MENU_H
#define RENDER_MENU_H

#include <wlr/types/wlr_buffer.h>
#include "config_loader.h"

#define MENU_WIDTH 210
#define MENU_ITEM_HEIGHT 36
#define MENU_SEP_HEIGHT 10
#define MENU_PADDING 10

int get_menu_height(void);
int get_menu_item_at(int rel_x, int rel_y);
struct wlr_buffer *render_menu_buffer(int hover_index);

#endif /* RENDER_MENU_H */
