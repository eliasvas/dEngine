#include "dcore.h"
#include "dui_renderer.h"
#include "dentity.h"
#include "dlog.h"

extern mu_Context ctx;
extern dgDevice dd;
extern dLogger engine_log;
//FEATURES TODO
//Skyboxes!
//Skeletal Animation
//Thread Pool + job graph!
//shader/texture CACHING!!!!!
//Engine logging
//Basic Sound (+ Audio compression/decompression)
//Render Graph

static  char logbuf[64000];
static   int logbuf_updated = 0;
static void write_log(const char *text) {
  if (logbuf[0]) { strcat(logbuf, "\n"); }
  strcat(logbuf, text);
  logbuf_updated = 1;
}

static void log_window(mu_Context *ctx) {
  if (mu_begin_window(ctx, "Log Window", mu_rect(350, 40, 300, 200))) {
    /* output text panel */
    mu_layout_row(ctx, 1, (int[]) { -1 }, -25);
    mu_begin_panel(ctx, "Log Output");
    mu_Container *panel = mu_get_current_container(ctx);
    mu_layout_row(ctx, 1, (int[]) { -1 }, -1);
    //mu_text(ctx, engine_log.logs[(i + engine_log.current_log) % MAX_LOGS].msg);
    for (u32 i = 0; i < MAX_LOGS; ++i)
    {
        dLogMessage *m = &engine_log.logs[(i + engine_log.current_log) % MAX_LOGS];
        write_log(m->msg);
    }
    mu_text(ctx, logbuf);
    if (logbuf_updated) {
      panel->scroll.y = panel->content_size.y;
      logbuf_updated = 0;
    }
    mu_end_panel(ctx);

    /* input textbox + submit button */
    static char buf[128];
    int submitted = 0;
    mu_layout_row(ctx, 2, (int[]) { -70, -1 }, 0);
    if (mu_textbox(ctx, buf, sizeof(buf)) & MU_RES_SUBMIT) {
      mu_set_focus(ctx, ctx->last_id);
      submitted = 1;
    }
    if (mu_button(ctx, "Submit")) { submitted = 1; }
    if (submitted) {
      //write_log(buf);
      dlog(NULL, buf);
      buf[0] = '\0';
    }

    mu_end_window(ctx);
  }
}


static void test_window(mu_Context *ctx) {
  /* do window */
  if (mu_begin_window(ctx, "Demo Window", mu_rect(40, 40, 150, 150))) {
    mu_Container *win = mu_get_current_container(ctx);
    win->rect.w = mu_max(win->rect.w, 240);
    win->rect.h = mu_max(win->rect.h, 200);

    /* window info */
    if (mu_header(ctx, "Window Info")) {
      mu_Container *win = mu_get_current_container(ctx);
      char buf[64];
      mu_layout_row(ctx, 2, (int[]) { 54, -1 }, 0);
      mu_label(ctx,"Position:");
      sprintf(buf, "%d, %d", win->rect.x, win->rect.y); mu_label(ctx, buf);
      mu_label(ctx, "Shad:");
      mu_checkbox(ctx, "On/Off", &dd.shadow_pass_active);
      mu_label(ctx, "Grid:");
      mu_checkbox(ctx, "On/Off", &dd.grid_active);


    }
    mu_end_window(ctx);
  }
}

static void component_window(mu_Context *ctx, dTransformCM *manager, dEntity e) {
  /* do window */
  u32 component_index = dtransform_cm_lookup(manager, e);
  dComponentDesc d = manager->component_desc;
  dTransform t = manager->data.world[component_index];

  if (mu_begin_window(ctx, "Component View", mu_rect(50,350, 200,200))) {
    mu_Container *win = mu_get_current_container(ctx);


    for (u32 i = 0; i < d.field_count; ++i){
      dComponentField *f = &d.fields_buf[i];
      u32 cnt=0;
      s32 layout_offsets[] = {0,20,40,60,80};
      if (mu_header(ctx, f->name))
      {
        mu_Container *win = mu_get_current_container(ctx);
        u32 items_num = 0;
        if (f->type == DCOMPONENT_FIELD_TYPE_F32)
          items_num = 1;
        else if (f->type == DCOMPONENT_FIELD_TYPE_VEC2)
          items_num = 2;
        else if (f->type == DCOMPONENT_FIELD_TYPE_VEC3)
          items_num = 3;
        else if (f->type == DCOMPONENT_FIELD_TYPE_VEC4)
          items_num = 4;
          
        
        mu_layout_row(ctx, items_num, layout_offsets, 0);
        for (u32 i = 0; i < items_num; ++i){
          void * addr =(void*)(&t) + f->offset + i * sizeof(f32);
          f32 *val = (f32*)addr;
          cnt+=mu_slider_ex(ctx, addr, -20.0f, 20.0f, 0, "%.0f", MU_OPT_ALIGNRIGHT);
        }
      }
      if (cnt)
          dtransform_cm_set_local(manager, component_index, t);
    }

    mu_end_window(ctx);
  }
}


void init(void)
{
    dcore_init();
}

extern dTransformCM transform_manager;
b32 update(void)
{
    dinput_update();

   mu_begin(&ctx);
   test_window(&ctx);
   component_window(&ctx, &transform_manager, (dEntity){0});
   mu_end(&ctx);
    
   return 1;
}

void destroy(void)
{
    dcore_destroy();
}

extern dProfiler global_profiler;
int main(void)
{
    init();
    while(update())
    {
        dcore_update();//update the state of the engine for each step
        dui_clear(mu_color(0, 0, 0, 255));
        mu_Command *cmd = NULL;
        while (mu_next_command(&ctx, &cmd)) {
            switch (cmd->type) {
                case MU_COMMAND_TEXT: dui_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
                case MU_COMMAND_RECT: dui_draw_rect(cmd->rect.rect, cmd->rect.color); break;
                case MU_COMMAND_ICON: dui_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
                case MU_COMMAND_CLIP: dui_set_clip_rect(cmd->clip.rect); break;
            }
        }

        /*
        dProfilerTag *tag = &global_profiler.tags[hmget(global_profiler.name_hash, hash_str("UPDATE"))];
        f32 ms_max = 0.f;
        f32 ms_min = FLT_MAX;
        for (u32 i = 0; i < MAX_SAMPLES_PER_NAME; ++i) {
            if (tag->samples[i] > ms_max)ms_max = tag->samples[i];
            if (tag->samples[i] < ms_min)ms_min = tag->samples[i];
        }
        for (u32 i = 0; i < MAX_SAMPLES_PER_NAME; ++i) {
            u32 index= (i +tag->next_sample_to_write) % MAX_SAMPLES_PER_NAME;
            //f32 factor = (tag->samples[i] - ms_min) / (ms_max - ms_min);
            f32 factor = tag->samples[index]/100.f;
            mu_Rect rect = {50 + 10 * index,150,10,-100*factor};
            mu_Color col = {255,255*(1-factor),255*(1-factor),255};
            dui_draw_rect(rect,col);
        }
        */
        dui_present();
    }


/*
    dJobDecl job_decl = {print_nonsense, arg00};
    dJobDecl job_decl2 = {print_number, &numberr};

    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_queue_add_job(&job_manager.job_queue, job_decl2);
    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_manager_work(&job_manager);
*/



    destroy();
    return 0;
}
