#include <opencv2/opencv.hpp>
#include "keymap.h"


int jsToKeyboardMap(int js_key)
{
    switch(js_key)
    {
        case JS_KEY_TOGGLE_JS: return TOGGLE_JS;
        case JS_KEY_VIDEOREC: return VIDEOREC;
        case JS_KEY_TOGGLE_SS: return TOGGLE_SS;
        case JS_KEY_TOGGLE_TH: return TOGGLE_SPEED;
        case JS_KEY_TOGGLE_CMD_OUT: return TOGGLE_CMD_OUT;
        case JS_KEY_TOGGLE_LOS: return TOGGLE_LOS;
        case JS_KEY_DEV_MODE: return DEV_MODE;
        case JS_KEY_DEV_TAB_LEFT : return DEV_TAB_LEFT;
        case JS_KEY_DEV_TAB_RIGHT: return DEV_TAB_RIGHT;
    }

    return 0;
}

