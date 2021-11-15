#include <opencv2/opencv.hpp>
#include "keymap.h"

command_msg cmd_fwd = {{COMMAND_MSG_ID}, DIR_FWD, 0};
command_msg cmd_bwd = {{COMMAND_MSG_ID}, DIR_BWD, 0};
command_msg cmd_lft = {{COMMAND_MSG_ID}, DIR_LFT, 0};
command_msg cmd_rgt = {{COMMAND_MSG_ID}, DIR_RGT, 0};
command_msg cmd_td01 = {{COMMAND_MSG_ID}, DIR_NONE, -1};
command_msg cmd_td10 = {{COMMAND_MSG_ID}, DIR_NONE, -10};
command_msg cmd_tu01 = {{COMMAND_MSG_ID}, DIR_NONE, 1};
command_msg cmd_tu10 = {{COMMAND_MSG_ID}, DIR_NONE, 10};
command_msg cmd_tmax = {{COMMAND_MSG_ID}, DIR_NONE, THROTTLE_STATE_MAX};
command_msg cmd_tmin = {{COMMAND_MSG_ID}, DIR_NONE, THROTTLE_EMERGENCY_BREAK};
command_msg cmd_none = {{0},              DIR_NONE, 0x00};

command_msg getCommandOfKey(int key, bool* error)
{
    switch(key)
    {
    case MOVE_FWD : *error = false; return cmd_fwd;
    case MOVE_BWD : *error = false; return cmd_bwd;
    case MOVE_LFT : *error = false; return cmd_lft;
    case MOVE_RGT : *error = false; return cmd_rgt;
    case THR_DW01 : *error = false; return cmd_td01;
    case THR_DW10 : *error = false; return cmd_td10;
    case THR_UP01 : *error = false; return cmd_tu01;
    case THR_UP10 : *error = false; return cmd_tu10;
    case THR_MAX  : *error = false; return cmd_tmax;
    case THR_ZERO : *error = false; return cmd_tmin;
    }

    *error = true;
    return cmd_none;
}

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

const char* getNameOfKey(int key)
{
    switch(key)
    {
    case MOVE_FWD : return "MOVE_FWD";
    case MOVE_BWD : return "MOVE_BWD";
    case MOVE_LFT : return "MOVE_LFT";
    case MOVE_RGT : return "MOVE_RGT";
    case THR_DW01 : return "THR_DW01";
    case THR_DW10 : return "THR_DW10";
    case THR_UP01 : return "THR_UP01";
    case THR_UP10 : return "THR_UP10";
    case THR_MAX  : return "THR_MAX";
    case THR_ZERO : return "THR_ZERO";
    default: return "NONE";
    }
}
