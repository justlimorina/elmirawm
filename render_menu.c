#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <drm_fourcc.h>
#include <wlr/interfaces/wlr_buffer.h>
#include "render_menu.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

struct custom_cairo_buffer {
	struct wlr_buffer base;
	cairo_surface_t *cairo_surface;
	void *data;
	size_t size;
};

static void buffer_destroy(struct wlr_buffer *wlr_buffer)
{
	struct custom_cairo_buffer *buf =
		wl_container_of(wlr_buffer, buf, base);
	cairo_surface_destroy(buf->cairo_surface);
	free(buf->data);
	free(buf);
}

static bool buffer_begin_data_ptr_access(struct wlr_buffer *wlr_buffer,
		uint32_t flags, void **data, uint32_t *format, size_t *stride)
{
	struct custom_cairo_buffer *buf =
		wl_container_of(wlr_buffer, buf, base);
	*data = buf->data;
	*format = DRM_FORMAT_ARGB8888;
	*stride = (uint32_t)cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, wlr_buffer->width);
	return true;
}

static void buffer_end_data_ptr_access(struct wlr_buffer *wlr_buffer)
{
}

static const struct wlr_buffer_impl buffer_impl = {
	.destroy = buffer_destroy,
	.begin_data_ptr_access = buffer_begin_data_ptr_access,
	.end_data_ptr_access = buffer_end_data_ptr_access,
};

int get_menu_height(void)
{
	int i, h = MENU_PADDING * 2;
	for (i = 0; i < g_config.menu_item_count; i++) {
		if (g_config.menu_items[i].action && strcmp(g_config.menu_items[i].action, "separator") == 0) {
			h += MENU_SEP_HEIGHT;
		} else {
			h += MENU_ITEM_HEIGHT;
		}
	}
	return h;
}

int get_menu_item_at(int rel_x, int rel_y)
{
	int i;
	int cur_y = MENU_PADDING;

	if (rel_x < 0 || rel_x >= MENU_WIDTH)
		return -1;

	for (i = 0; i < g_config.menu_item_count; i++) {
		int item_h = (g_config.menu_items[i].action && strcmp(g_config.menu_items[i].action, "separator") == 0)
			? MENU_SEP_HEIGHT : MENU_ITEM_HEIGHT;

		if (rel_y >= cur_y && rel_y < cur_y + item_h) {
			if (g_config.menu_items[i].action && strcmp(g_config.menu_items[i].action, "separator") == 0)
				return -1;
			return i;
		}
		cur_y += item_h;
	}
	return -1;
}

static void draw_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r)
{
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + w - r, y + r, r, -M_PI_2, 0);
	cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI_2);
	cairo_arc(cr, x + r, y + h - r, r, M_PI_2, M_PI);
	cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI_2);
	cairo_close_path(cr);
}

struct wlr_buffer *render_menu_buffer(int hover_index)
{
	int width = MENU_WIDTH;
	int height = get_menu_height();
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	size_t size = (size_t)stride * height;
	void *data = calloc(1, size);
	cairo_surface_t *surface;
	cairo_t *cr;
	PangoLayout *layout;
	PangoFontDescription *font_desc;
	struct custom_cairo_buffer *buf;
	int i, cur_y;

	if (!data)
		return NULL;

	surface = cairo_image_surface_create_for_data(
		data, CAIRO_FORMAT_ARGB32, width, height, stride);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		free(data);
		return NULL;
	}

	cr = cairo_create(surface);
	layout = pango_cairo_create_layout(cr);

	font_desc = pango_font_description_from_string("Roboto Medium 11");
	pango_layout_set_font_description(layout, font_desc);
	pango_font_description_free(font_desc);

	/* 1. Background: MD3 Surface Container High (#2B2930) */
	draw_rounded_rect(cr, 1.0, 1.0, width - 2.0, height - 2.0, (double)g_config.border_radius);
	cairo_set_source_rgba(cr, 0.17f, 0.16f, 0.19f, 0.96f);
	cairo_fill_preserve(cr);

	/* 2. Border: Outline Variant (#49454F) */
	cairo_set_source_rgba(cr, 0.29f, 0.27f, 0.31f, 0.80f);
	cairo_set_line_width(cr, 1.0);
	cairo_stroke(cr);

	/* 3. Render Items */
	cur_y = MENU_PADDING;
	for (i = 0; i < g_config.menu_item_count; i++) {
		if (g_config.menu_items[i].action && strcmp(g_config.menu_items[i].action, "separator") == 0) {
			/* Draw separator line */
			cairo_set_source_rgba(cr, 0.29f, 0.27f, 0.31f, 0.60f);
			cairo_set_line_width(cr, 1.0);
			cairo_move_to(cr, MENU_PADDING, cur_y + MENU_SEP_HEIGHT / 2.0);
			cairo_line_to(cr, width - MENU_PADDING, cur_y + MENU_SEP_HEIGHT / 2.0);
			cairo_stroke(cr);
			cur_y += MENU_SEP_HEIGHT;
		} else {
			int item_x = MENU_PADDING;
			int item_w = width - MENU_PADDING * 2;
			int item_h = MENU_ITEM_HEIGHT;
			int text_w, text_h;

			if (i == hover_index) {
				/* Hover background: Surface Variant (#4A4458) with 8px radius */
				draw_rounded_rect(cr, item_x, cur_y + 2, item_w, item_h - 4, 8.0);
				cairo_set_source_rgba(cr, 0.29f, 0.27f, 0.35f, 0.90f);
				cairo_fill(cr);
			}

			/* Text: On Surface (#E6E1E5) */
			cairo_set_source_rgba(cr, 0.90f, 0.88f, 0.90f, 1.0f);
			pango_layout_set_text(layout, g_config.menu_items[i].label ? g_config.menu_items[i].label : "", -1);

			pango_layout_get_pixel_size(layout, &text_w, &text_h);
			cairo_move_to(cr, item_x + 12, cur_y + (item_h - text_h) / 2.0);
			pango_cairo_show_layout(cr, layout);

			cur_y += item_h;
		}
	}

	g_object_unref(layout);
	cairo_destroy(cr);

	buf = calloc(1, sizeof(*buf));
	if (!buf) {
		cairo_surface_destroy(surface);
		free(data);
		return NULL;
	}

	wlr_buffer_init(&buf->base, &buffer_impl, width, height);
	buf->cairo_surface = surface;
	buf->data = data;
	buf->size = size;

	return &buf->base;
}
