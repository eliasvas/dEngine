#ifndef __DINPUT__
#define __DINPUT__
#include "tools.h"

typedef enum dKeyboardKey
{
    DK_A = 1,
    DK_B,
    DK_C,
    DK_D,
    DK_E,
    DK_F,
    DK_G,
    DK_H,
    DK_I,
    DK_J,
    DK_K,
    DK_L,
    DK_M,
    DK_N,
    DK_O,
    DK_P,
    DK_Q,
    DK_R,
    DK_S,
    DK_T,
    DK_U,
    DK_V,
    DK_W,
    DK_X,
    DK_Y,
    DK_Z,

    DK_0,
    DK_1,
    DK_2,
    DK_3,
    DK_4,
    DK_5,
    DK_6,
    DK_7,
    DK_8,
    DK_9,

    DK_NUMPAD_0,
    DK_NUMPAD_1,
    DK_NUMPAD_2,
    DK_NUMPAD_3,
    DK_NUMPAD_4,
    DK_NUMPAD_5,
    DK_NUMPAD_6,
    DK_NUMPAD_7,
    DK_NUMPAD_8, DK_NUMPAD_9,
    DK_NUMPAD_MULTIPLY,
    DK_NUMPAD_ADD,
    DK_NUMPAD_SUBTRACT,
    DK_NUMPAD_DECIMAL,
    DK_NUMPAD_DIVIDE,

    DK_LEFT,
    DK_UP,
    DK_RIGHT,
    DK_DOWN,

    DK_BACKSPACE,
    DK_TAB,
    DK_CTRL,
    DK_RETURN,
    DK_SPACE,
    DK_LSHIFT,
    DK_RSHIFT,
    DK_LCONTROL,
    DK_RCONTROL,
    DK_ALT,
    DK_LSUPER,
    DK_RSUPER,
    DK_CAPSLOCK,
    DK_ESCAPE,
    DK_PAGEUP,
    DK_PAGEDOWN,
    DK_HOME,
    DK_END,
    DK_INSERT,
    DK_DELETE,
    DK_PAUSE,
    DK_NUMLOCK,
    DK_PRINTSCREEN,

    DK_F1,
    DK_F2,
    DK_F3,
    DK_F4,
    DK_F5,
    DK_F6,
    DK_F7,
    DK_F8,
    DK_F9,
    DK_F10,
    DK_F11,
    DK_F12,


    DK_MAX
}dKeyboardKey;

b32 dkey_pressed(dKeyboardKey k);
b32 dkey_released(dKeyboardKey k);
b32 dkey_up(dKeyboardKey k);
b32 dkey_down(dKeyboardKey k);


//inits all state needed to capture input
void dinput_init(void);

//updates input state (should be called once every frame)
void dinput_update(void);

#endif