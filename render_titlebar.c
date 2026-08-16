#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <drm_fourcc.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <wlr/interfaces/wlr_buffer.h>
#include "render_titlebar.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#include "config_loader.h"

/*
 * Custom CPU-backed wlr_buffer implementation.
 * GBM (GPU) buffers don't support DATA_PTR_ACCESS for CPU rendering,
 * so we allocate our own ARGB8888 pixel buffer in RAM.
 */
struct titlebar_buffer {
	struct wlr_buffer base;
	void *data;
	size_t stride;
};

static void titlebar_buffer_destroy(struct wlr_buffer *wlr_buf)
{
	struct titlebar_buffer *buf = wl_container_of(wlr_buf, buf, base);
	free(buf->data);
	free(buf);
}

static bool titlebar_buffer_begin_data_ptr_access(struct wlr_buffer *wlr_buf,
		uint32_t flags, void **data, uint32_t *format, size_t *stride)
{
	struct titlebar_buffer *buf = wl_container_of(wlr_buf, buf, base);
	*data = buf->data;
	*format = DRM_FORMAT_ARGB8888;
	*stride = buf->stride;
	return true;
}

static void titlebar_buffer_end_data_ptr_access(struct wlr_buffer *wlr_buf)
{
	/* no-op for CPU buffer */
}

static const struct wlr_buffer_impl titlebar_buffer_impl = {
	.destroy = titlebar_buffer_destroy,
	.begin_data_ptr_access = titlebar_buffer_begin_data_ptr_access,
	.end_data_ptr_access = titlebar_buffer_end_data_ptr_access,
};

static struct titlebar_buffer *titlebar_buffer_create(int width, int height)
{
	struct titlebar_buffer *buf = calloc(1, sizeof(*buf));
	if (!buf)
		return NULL;

	buf->stride = (size_t)width * 4;
	buf->data = calloc((size_t)height, buf->stride);
	if (!buf->data) {
		free(buf);
		return NULL;
	}

	wlr_buffer_init(&buf->base, &titlebar_buffer_impl, width, height);
	return buf;
}

static void rounded_path(cairo_t *cr, double x, double y, double w, double h, double r)
{
	if (r > h / 2.0)
		r = h / 2.0;
	if (r * 2 > w)
		r = w / 2.0;
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + w - r, y + r, r, -M_PI_2, 0);
	cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI_2);
	cairo_arc(cr, x + r, y + h - r, r, M_PI_2, M_PI);
	cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI_2);
	cairo_close_path(cr);
}

static cairo_t *begin_paint(struct titlebar_buffer *tb, cairo_surface_t **psurface)
{
	cairo_t *cr;
	*psurface = cairo_image_surface_create_for_data(
			tb->data, CAIRO_FORMAT_ARGB32, tb->base.width, tb->base.height,
			(int)tb->stride);
	cr = cairo_create(*psurface);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	return cr;
}

static void end_paint(cairo_t *cr, cairo_surface_t *surface)
{
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

struct wlr_buffer *
render_border_buffer(int width, int height, int bw, const float color[4], double radius)
{
	struct titlebar_buffer *tb = NULL;
	cairo_surface_t *surface = NULL;
	cairo_t *cr = NULL;
	double half_bw = 0.0, x = 0.0, y = 0.0, w = 0.0, h = 0.0, r = 0.0;

	if (width <= 0 || height <= 0 || bw <= 0 || !color)
		return NULL;

	tb = titlebar_buffer_create(width, height);
	if (!tb)
		return NULL;

	cr = begin_paint(tb, &surface);

	/* 1. Fill exterior corners outside rounded rectangle to mask sharp client surface corners */
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_rectangle(cr, 0, 0, width, height);
	rounded_path(cr, 0, 0, width, height, radius);
	cairo_set_source_rgba(cr, 0.078, 0.071, 0.094, 0.95);
	cairo_fill(cr);

	/* 2. Draw rounded border stroke line */
	half_bw = bw / 2.0;
	x = half_bw;
	y = half_bw;
	w = width - bw;
	h = height - bw;
	r = radius - half_bw;
	if (r < 1.0) r = 1.0;

	cairo_set_line_width(cr, bw);
	cairo_set_source_rgba(cr, (double)color[0], (double)color[1], (double)color[2], (double)color[3]);

	rounded_path(cr, x, y, w, h, r);
	cairo_stroke(cr);

	end_paint(cr, surface);

	return &tb->base;
}

struct wlr_buffer *
render_shadow_buffer(int width, int height, int active)
{
	struct titlebar_buffer *tb = NULL;
	cairo_surface_t *surface = NULL;
	cairo_t *cr = NULL;
	int margin = 16;
	int sw = width + 2 * margin;
	int sh = height + 2 * margin;
	double max_alpha = active ? 0.28 : 0.12;
	int spread = active ? 12 : 6;
	double base_r = 12.0;
	int s;

	if (width <= 0 || height <= 0)
		return NULL;

	tb = titlebar_buffer_create(sw, sh);
	if (!tb)
		return NULL;

	cr = begin_paint(tb, &surface);

	/* Render multi-layer ambient shadow for smooth MD3 elevation */
	for (s = spread; s >= 0; s--) {
		double alpha = max_alpha * (1.0 - (double)s / (double)(spread + 1));
		double exp = (double)s;
		double x = margin - exp;
		double y = margin - exp;
		double w = width + 2.0 * exp;
		double h = height + 2.0 * exp;
		double r = base_r + exp;

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, alpha / (double)(spread + 1));

		rounded_path(cr, x, y, w, h, r);
		cairo_fill(cr);
	}

	end_paint(cr, surface);

	return &tb->base;
}

