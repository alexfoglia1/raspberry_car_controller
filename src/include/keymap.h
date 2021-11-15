#ifndef KEYMAP_H
#define KEYMAP_H

#include "defs.h"

/** Keys for local usage **/
#define TOGGLE_SPEED (int)'s'
#define TOGGLE_LOS (int)'l'
#define TOGGLE_TGT (int)'m'
#define TOGGLE_CMD_OUT (int)'c'
#define TOGGLE_MENU (int)'h'
#define TOGGLE_SS   (int)'y'
#define TOGGLE_JS   (int)'j'
#define VIDEOREC (int)'v'
#define DEV_MODE (int)'e'
#define DEV_TAB_RIGHT (int)'g'
#define DEV_TAB_LEFT (int)'f'

/** Js jeys for local usage **/
#define JS_KEY_TOGGLE_JS 8
#define JS_KEY_VIDEOREC  9
#define JS_KEY_TOGGLE_TH 0
#define JS_KEY_TOGGLE_SS 1
#define JS_KEY_TOGGLE_CMD_OUT 2
#define JS_KEY_TOGGLE_LOS 3
/* SQUARE UNUSED ATM (4) */
#define JS_KEY_DEV_MODE 5
#define JS_KEY_DEV_TAB_RIGHT 12
#define JS_KEY_DEV_TAB_LEFT 11

/** Keys to invoke remote cmd **/
#define MOVE_FWD (int)'w'
#define MOVE_BWD (int)'x'
#define MOVE_LFT (int)'a'
#define MOVE_RGT (int)'d'
#define THR_UP10 (int)'p'
#define THR_DW10 (int)'q'
#define THR_ZERO 8
#define THR_MAX  32
#define THR_UP01 (int)'n'
#define THR_DW01 (int)'b'


command_msg getCommandOfKey(int key, bool* error);
const char* getNameOfKey(int key);
int jsToKeyboardMap(int js_key);

#endif // KEYMAP_H
