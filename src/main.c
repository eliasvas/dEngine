#include "dcore.h"
#include "dui_renderer.h"

extern mu_Context ctx;
extern dgDevice dd;
//FEATURES TODO
//Thread Pool + job graph!
//Render Targets should have customizable widths/height AND formats for each texture in the RT
//make the engine self compile (no external dependencies) to a single executable (pack shaders in a .inl basically)
//shader/texture CACHING!!!!!
//Multithreading!!!!
//-Engine logging
//-Profiling
//-Basic Sound (+ Audio compression/decompression)
//Textures and RTs should have different formats and pipelines should know about it! (also look at pipe barriers and synchronization!)



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


void init(void)
{
    dcore_init();
}

b32 update(void)
{
    dinput_update();

   mu_begin(&ctx);
   test_window(&ctx);
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
