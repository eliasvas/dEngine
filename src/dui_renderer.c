#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <assert.h>
#include "dui_renderer.h"
#include "../ext/microui/atlas.inl"
#include "dgfx.h"
#include "dcore.h"

#define BUFFER_SIZE 16384


// pos/norm/tex --> pos/col/tex
static dgVertex vertices[BUFFER_SIZE]; 
static u32 index_buf[BUFFER_SIZE *  6];

static dgBuffer vbo;
static dgBuffer ibo;

static s32 width  = 600;
static s32 height = 400;
static s32 buf_idx;

extern dgTexture t; //this is the atlas texture, maybe we should load it here??? idk
extern dgDevice dd;
extern dWindow dwin;

void dui_init(void) {
}


static void flush(void) {
  if (buf_idx == 0) { return; }

  buf_idx = 0;
}


static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color) {
  if (buf_idx == BUFFER_SIZE) { flush(); }

  u32   index_idx = buf_idx *  6; //6 elements for each vertex
  u32   element_idx = buf_idx * 4;
  u32   vert_idx = buf_idx * 4;
  buf_idx++;

  /* update texture buffer */
  f32 x = src.x / (f32) ATLAS_WIDTH;
  f32 y = src.y / (f32) ATLAS_HEIGHT;
  f32 w = src.w / (f32) ATLAS_WIDTH;
  f32 h = src.h / (f32) ATLAS_HEIGHT;
  vertices[vert_idx + 0].tex_coord = v2(x,y);
  vertices[vert_idx + 1].tex_coord = v2(x+w,y);
  vertices[vert_idx + 2].tex_coord = v2(x,y+h);
  vertices[vert_idx + 3].tex_coord = v2(x+w,y+h);

  /* update vertex buffer */ 
  //I encode Alpha as 3rd component of position, why??? idk i dont want another vertex type
  vertices[vert_idx + 0].pos = v3(dst.x,dst.y,color.a / (f32)255);
  vertices[vert_idx + 1].pos = v3(dst.x + dst.w,dst.y,color.a / (f32)255);
  vertices[vert_idx + 2].pos = v3(dst.x,dst.y + dst.h,color.a / (f32)255);
  vertices[vert_idx + 3].pos = v3(dst.x + dst.w,dst.y + dst.h,color.a / (f32)255);

  /* update color buffer */
  vertices[vert_idx + 0].normal = v3(color.r / (f32)255, color.g / (f32)255, color.b / (f32)255);
  vertices[vert_idx + 1].normal = v3(color.r / (f32)255, color.g / (f32)255, color.b / (f32)255);
  vertices[vert_idx + 2].normal = v3(color.r / (f32)255, color.g / (f32)255, color.b / (f32)255);
  vertices[vert_idx + 3].normal = v3(color.r / (f32)255, color.g / (f32)255, color.b / (f32)255);

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
  if (!buf_idx)return;
  //printf("dui_present\n");
  dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&vbo, buf_idx * 4 * sizeof(dgVertex), vertices);
	
	
	
	//create index buffer
	dg_create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&ibo, buf_idx * 6 * sizeof(u32), index_buf);


  dg_rendering_begin(&dd, NULL, 1, NULL, FALSE);
  dg_set_viewport(&dd, 0,0, dd.swap.extent.width, dd.swap.extent.height);
  dg_set_scissor(&dd, 0,0, dd.swap.extent.width, dd.swap.extent.height);


  //UI drawcall 
  dg_bind_pipeline(&dd, &dd.dui_pipe);
  dgBuffer buffers[] = {vbo};
  u64 offsets[] = {0};
  dg_bind_vertex_buffers(&dd, buffers, offsets, 1);
  dg_bind_index_buffer(&dd, &ibo);

  f32 data[4] = {main_window.width,main_window.height,0.2,0.2};
  dg_set_desc_set(&dd,&dd.dui_pipe, data, sizeof(data), 1);
  dg_set_desc_set(&dd,&dd.dui_pipe, &t, 1, 2);
  dg_draw(&dd, 4,ibo.size/sizeof(u32));

  dg_rendering_end(&dd);



  flush();
}
