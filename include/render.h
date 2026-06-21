#ifndef OLEDTTY_RENDER_H
#define OLEDTTY_RENDER_H

#include "types.h"
#include "framebuffer.h"
#include "viewport.h"

void render_frame(framebuffer_t *fb,
                  const viewport_t *vp,
                  bool cursor_visible);

#endif
