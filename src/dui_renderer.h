#ifndef DUI_RENDERER_H
#define DUI_RENDERER_H

#include "../ext/microui/microui.h"
#include "tools.h"

void dui_init(void);
void dui_draw_rect(mu_Rect rect, mu_Color color);
void dui_draw_text(const char *text, mu_Vec2 pos, mu_Color color);
void dui_draw_icon(s32 id, mu_Rect rect, mu_Color color);
s32 dui_get_text_width(const char *text, int len);
s32 dui_get_text_height(void);
void dui_set_clip_rect(mu_Rect rect);
void dui_clear(mu_Color color);
void dui_present(void);

#endif