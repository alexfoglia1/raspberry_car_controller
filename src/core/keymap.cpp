#include "keymap.h"

int jsToKeyboardMap(int js_key)
{
    switch(js_key)
    {
        case JS_KEY_TOGGLE_JS: return TOGGLE_JS;
        case JS_KEY_VIDEOREC: return VIDEOREC;
        case JS_KEY_TOGGLE_SS: return TOGGLE_SS;
        case JS_KEY_TOGGLE_TH: return TOGGLE_SPEED;
        case JS_KEY_TOGGLE_LOS: return TOGGLE_LOS;
        case JS_KEY_DEV_MODE: return DEV_MODE;
    }

    return 0;
}
