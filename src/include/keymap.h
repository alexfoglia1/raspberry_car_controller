#ifndef KEYMAP_H
#define KEYMAP_H

#include "defs.h"

/** Keys for local usage **/
#define TOGGLE_SPEED (int)'s'
#define TOGGLE_LOS (int)'l'
#define TOGGLE_TGT (int)'m'
#define TOGGLE_MENU (int)'h'
#define TOGGLE_SS   (int)'y'
#define TOGGLE_JS   (int)'j'
#define VIDEOREC (int)'v'
#define DEV_MODE (int)'e'
#define DEV_TAB_RIGHT (int)'g'
#define DEV_TAB_LEFT (int)'f'
#define RIGHT_ARROW 83
#define LEFT_ARROW 81
#define UP_ARROW 82
#define DOWN_ARROW 84
#define ESCAPE 27

/** Js jeys for local usage **/
#define JS_KEY_TOGGLE_JS 8
#define JS_KEY_VIDEOREC  9
#define JS_KEY_TOGGLE_TH 0
#define JS_KEY_TOGGLE_SS 1
#define JS_KEY_TOGGLE_LOS 3

/* SQUARE UNUSED ATM (4) */
#define JS_KEY_DEV_MODE 5
#define JS_KEY_DEV_TAB_RIGHT 12
#define JS_KEY_DEV_TAB_LEFT 11


int jsToKeyboardMap(int js_key);

#endif // KEYMAP_H
