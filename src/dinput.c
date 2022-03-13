#include "dinput.h"

//used for input system update
#include "SDL.h"



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
    while( SDL_PollEvent( &e ) != 0)
    {
        //we don't want repeating keys to be counted
        if (e.key.repeat == 1)continue;

        switch (e.type)
        {
            case SDL_QUIT:
                exit(1);
            case SDL_KEYDOWN:
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
                dis.keys[DK_LMB + e.button.button - 1] = 1;
                break;
            case SDL_MOUSEBUTTONUP:
                dis.keys[DK_LMB + e.button.button - 1] = 0;
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

