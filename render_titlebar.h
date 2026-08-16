#ifndef RENDER_TITLEBAR_H
#define RENDER_TITLEBAR_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>

struct wlr_buffer *render_border_buffer(int width, int height, int bw, const float color[4], double radius);
struct wlr_buffer *render_shadow_buffer(int width, int height, int active);

#endif /* RENDER_TITLEBAR_H */
