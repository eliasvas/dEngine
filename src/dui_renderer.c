#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <assert.h>
#include "dui_renderer.h"
#include "../ext/microui/atlas.inl"
#include "dgfx.h"

#define BUFFER_SIZE 16384

static f32 tex_buf[BUFFER_SIZE *  8];
static f32 vert_buf[BUFFER_SIZE *  8];
static  u8 color_buf[BUFFER_SIZE * 16];
static u32 index_buf[BUFFER_SIZE *  6];

static s32 width  = 800;
static s32 height = 600;
static s32 buf_idx;

extern dgTexture t; //this is the atlas texture, maybe we should load it here??? idk

void dui_init(void) {
}


static void flush(void) {
  if (buf_idx == 0) { return; }

  buf_idx = 0;
}


static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color) {
  if (buf_idx == BUFFER_SIZE) { flush(); }

  u32 texvert_idx = buf_idx *  8;
  u32   color_idx = buf_idx * 16;
  u32 element_idx = buf_idx *  4;
  u32   index_idx = buf_idx *  6;
  buf_idx++;

  /* update texture buffer */
  f32 x = src.x / (f32) ATLAS_WIDTH;
  f32 y = src.y / (f32) ATLAS_HEIGHT;
  f32 w = src.w / (f32) ATLAS_WIDTH;
  f32 h = src.h / (f32) ATLAS_HEIGHT;
  tex_buf[texvert_idx + 0] = x;
  tex_buf[texvert_idx + 1] = y;
  tex_buf[texvert_idx + 2] = x + w;
  tex_buf[texvert_idx + 3] = y;
  tex_buf[texvert_idx + 4] = x;
  tex_buf[texvert_idx + 5] = y + h;
  tex_buf[texvert_idx + 6] = x + w;
  tex_buf[texvert_idx + 7] = y + h;

  /* update vertex buffer */
  vert_buf[texvert_idx + 0] = dst.x;
  vert_buf[texvert_idx + 1] = dst.y;
  vert_buf[texvert_idx + 2] = dst.x + dst.w;
  vert_buf[texvert_idx + 3] = dst.y;
  vert_buf[texvert_idx + 4] = dst.x;
  vert_buf[texvert_idx + 5] = dst.y + dst.h;
  vert_buf[texvert_idx + 6] = dst.x + dst.w;
  vert_buf[texvert_idx + 7] = dst.y + dst.h;

  /* update color buffer */
  memcpy(color_buf + color_idx +  0, &color, 4);
  memcpy(color_buf + color_idx +  4, &color, 4);
  memcpy(color_buf + color_idx +  8, &color, 4);
  memcpy(color_buf + color_idx + 12, &color, 4);

  /* update index buffer */
  index_buf[index_idx + 0] = element_idx + 0;
  index_buf[index_idx + 1] = element_idx + 1;
  index_buf[index_idx + 2] = element_idx + 2;
  index_buf[index_idx + 3] = element_idx + 2;
  index_buf[index_idx + 4] = element_idx + 3;
  index_buf[index_idx + 5] = element_idx + 1;
}


void dui_draw_rect(mu_Rect rect, mu_Color color) {
  push_quad(rect, atlas[ATLAS_WHITE], color);
}


void dui_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
  mu_Rect dst = { pos.x, pos.y, 0, 0 };
  for (const char *p = text; *p; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    mu_Rect src = atlas[ATLAS_FONT + chr];
    dst.w = src.w;
    dst.h = src.h;
    push_quad(dst, src, color);
    dst.x += dst.w;
  }
}


void dui_draw_icon(int id, mu_Rect rect, mu_Color color) {
  mu_Rect src = atlas[id];
  int x = rect.x + (rect.w - src.w) / 2;
  int y = rect.y + (rect.h - src.h) / 2;
  push_quad(mu_rect(x, y, src.w, src.h), src, color);
}


int dui_get_text_width(const char *text, int len) {
  int res = 0;
  for (const char *p = text; *p && len--; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    res += atlas[ATLAS_FONT + chr].w;
  }
  return res;
}


int dui_get_text_height(void) {
  return 18;
}


void dui_set_clip_rect(mu_Rect rect) {
  flush();
  //glScissor(rect.x, height - (rect.y + rect.h), rect.w, rect.h);
}


void dui_clear(mu_Color clr) {
  flush();
  //glClearColor(clr.r / 255., clr.g / 255., clr.b / 255., clr.a / 255.);
  //glClear(GL_COLOR_BUFFER_BIT);
}


void dui_present(void) {
  flush();
}
