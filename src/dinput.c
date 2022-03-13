#include "dinput.h"

//used for input system update
#include "SDL.h"



//The input state of the engine for the current frame (its a singleton!)
typedef struct dInputState
{
    dKeyboardKey keys0[DK_MAX];
    dKeyboardKey keys1[DK_MAX];
    //pointers to the arrays of keys so we don't have to copy one array each frame (current --copy--> prev)
    dKeyboardKey *keys, *prev_keys;
    b32 active_key;

    s32 mouse_pos_x, mouse_pos,y;
    u32 mouse_state;
}dInputState;

//This is the input singleton
dInputState dis;

void dinput_init(void)
{
    memset(&dis.keys0, 0, DK_MAX * sizeof(dis.keys0[0]));
    memset(&dis.keys1, 0, DK_MAX * sizeof(dis.keys1[0]));
}

void dinput_update(void)
{
    dis.keys = (dis.active_key) ? dis.keys1 : dis.keys0;
    dis.prev_keys = (dis.active_key) ? dis.keys0 : dis.keys1;
    dis.active_key = (dis.active_key == 0) ? 1 : 0;


    //First we set the leys as previous keys, because if there is no event, 
    //no state has changed, so the previous state should equal the now state of input
    memcpy(dis.keys,dis.prev_keys, sizeof(dis.keys[0]) * DK_MAX);


    SDL_Event e;
    while( SDL_PollEvent( &e ) != 0)
    {
        //we don't want repeating keys to be counted
        if (e.key.repeat == 1)continue;
        if (e.type == SDL_QUIT)
        {
            exit(1);
        }
        if( e.type == SDL_KEYDOWN )
        {
            switch( e.key.keysym.sym )
            {
                case SDLK_a:
                dis.keys[DK_A] = 1;
                break;

                case SDLK_DOWN:
                break;

                case SDLK_LEFT:
                break;

                case SDLK_RIGHT:
                break;

                default:
                break;
            }
        }
        if( e.type == SDL_KEYUP)
        {
            switch( e.key.keysym.sym )
            {
                case SDLK_a:
                dis.keys[DK_A] = 0;
                break;

                case SDLK_DOWN:
                break;

                case SDLK_LEFT:
                break;

                case SDLK_RIGHT:
                break;

                default:
                break;
            }
        }
    }
}

b32 dkey_pressed(dKeyboardKey k)
{
    return (dis.keys[k] && (dis.prev_keys[k] == 0));
}

b32 dkey_released(dKeyboardKey k)
{

    return (dis.prev_keys[k] && (dis.keys[k] == 0));
}

b32 dkey_up(dKeyboardKey k)
{

    return (dis.keys[k] == 0);
}

b32 dkey_down(dKeyboardKey k)
{
    return (dis.keys[k]);
}

