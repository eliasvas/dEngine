#include "dinput.h"
#include "tools.h"
#include "../ext/microui/microui.h"

//used for input system update
#include "SDL.h"

static const char mu_button_map[256] = {
  [ SDL_BUTTON_LEFT   & 0xff ] =  MU_MOUSE_LEFT,
  [ SDL_BUTTON_RIGHT  & 0xff ] =  MU_MOUSE_RIGHT,
  [ SDL_BUTTON_MIDDLE & 0xff ] =  MU_MOUSE_MIDDLE,
};

static const char mu_key_map[256] = {
  [ SDLK_LSHIFT       & 0xff ] = MU_KEY_SHIFT,
  [ SDLK_RSHIFT       & 0xff ] = MU_KEY_SHIFT,
  [ SDLK_LCTRL        & 0xff ] = MU_KEY_CTRL,
  [ SDLK_RCTRL        & 0xff ] = MU_KEY_CTRL,
  [ SDLK_LALT         & 0xff ] = MU_KEY_ALT,
  [ SDLK_RALT         & 0xff ] = MU_KEY_ALT,
  [ SDLK_RETURN       & 0xff ] = MU_KEY_RETURN,
  [ SDLK_BACKSPACE    & 0xff ] = MU_KEY_BACKSPACE,
};



extern mu_Context ctx; 

//The input state of the engine for the current frame (its a singleton!)
typedef struct dInputState
{
    dKey keys[DK_MAX];
    dKey prev_keys[DK_MAX];


    s32 mouse_pos_x, mouse_pos_y;
}dInputState;

//This is the input singleton
dInputState dis;

void dinput_init(void)
{
    memset(&dis.keys, 0, DK_MAX * sizeof(dis.keys[0]));
    memset(&dis.prev_keys, 0, DK_MAX * sizeof(dis.keys[0]));
}

void dinput_update(void)
{
    SDL_PumpEvents();
    memcpy(dis.prev_keys,dis.keys, sizeof(dis.keys[0]) * DK_MAX);

    SDL_Event e;
    s32 b,c;
    while( SDL_PollEvent( &e ) != 0)
    {
        //we don't want repeating keys to be counted
        if (e.key.repeat == 1)continue;

        switch (e.type)
        {
            case SDL_QUIT:
                exit(1);
            case SDL_KEYDOWN:
                c = mu_key_map[e.key.keysym.sym & 0xff];
                if (c) { mu_input_keydown(&ctx, c); }

                if (e.key.keysym.sym >= '0' && e.key.keysym.sym <= '9')
                {
                    dis.keys[DK_0 + e.key.keysym.sym - '0'] = 1;
                    continue;
                }
                if (e.key.keysym.sym >= 'a' && e.key.keysym.sym <='z')
                {
                    dis.keys[DK_A + e.key.keysym.sym - 'a'] = 1;
                    continue;
                }
                switch( e.key.keysym.sym )
                {
                    case SDLK_UP:
                    dis.keys[DK_UP] = 1;
                    break;
                    case SDLK_DOWN:
                    dis.keys[DK_DOWN] = 1;
                    break;
                    case SDLK_LEFT:
                    dis.keys[DK_LEFT] = 1;
                    break;
                    case SDLK_RIGHT:
                    dis.keys[DK_RIGHT] = 1;
                    break;
                    default:
                    break;
                }
                break;
            case SDL_KEYUP:
                c = mu_key_map[e.key.keysym.sym & 0xff];
                if (c) { mu_input_keyup(&ctx, c); }
                if (e.key.keysym.sym >= '0' && e.key.keysym.sym <= '9')
                {
                    dis.keys[DK_0 + e.key.keysym.sym - '0'] = 0;
                    continue;
                }
                if (e.key.keysym.sym >= 'a' && e.key.keysym.sym <='z')
                {
                    dis.keys[DK_A + e.key.keysym.sym - 'a'] = 0;
                    continue;
                }
                switch( e.key.keysym.sym )
                {
                    case SDLK_UP:
                    dis.keys[DK_UP] = 0;
                    break;
                    case SDLK_DOWN:
                    dis.keys[DK_DOWN] = 0;
                    break;
                    case SDLK_LEFT:
                    dis.keys[DK_LEFT] = 0;
                    break;
                    case SDLK_RIGHT:
                    dis.keys[DK_RIGHT] = 0;
                    break;
                    default:
                    break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                b = mu_button_map[e.button.button & 0xff];
                if (b) { mu_input_mousedown(&ctx, e.button.x, e.button.y, b);}
                dis.keys[DK_LMB + e.button.button - 1] = 1;
                break;
            case SDL_MOUSEBUTTONUP:
                b = mu_button_map[e.button.button & 0xff];
                if (b) { mu_input_mouseup(&ctx, e.button.x, e.button.y, b);}
                dis.keys[DK_LMB + e.button.button - 1] = 0;
                break;
            case SDL_MOUSEMOTION:
                mu_input_mousemove(&ctx, e.motion.x, e.motion.y);
                break;
            case SDL_TEXTINPUT:
                mu_input_text(&ctx, e.text.text);
                break;
            default:break;
        }
    }

    SDL_GetMouseState(&dis.mouse_pos_x, &dis.mouse_pos_y);
    //printf("mouse pos: %i %i\n", dis.mouse_pos_x, dis.mouse_pos_y);
}

ivec2 dinput_get_mouse_pos(void)
{
    return (ivec2){dis.mouse_pos_x, dis.mouse_pos_y};
}
b32 dkey_pressed(dKey k)
{
    return (dis.keys[k] && (dis.prev_keys[k] == 0));
}

b32 dkey_released(dKey k)
{

    return (dis.prev_keys[k] && (dis.keys[k] == 0));
}

b32 dkey_up(dKey k)
{

    return (dis.keys[k] == 0);
}

b32 dkey_down(dKey k)
{
    return (dis.keys[k]);
}

