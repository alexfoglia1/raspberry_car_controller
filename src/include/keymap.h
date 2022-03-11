#ifndef KEYMAP_H
#define KEYMAP_H

#include "defs.h"

/** Keys for local usage **/
#define TOGGLE_SPEED 83
#define TOGGLE_LOS 76
#define TOGGLE_TGT 77
#define TOGGLE_SS  89
#define VIDEOREC  86
#define TRACKER_IDLE 74
#define TRACKER_ACQUIRE  69
#define TRACKER_RUN 72
#define RIGHT_ARROW 16777236
#define LEFT_ARROW 16777234
#define UP_ARROW 16777235
#define DOWN_ARROW 16777237
#define ESCAPE 16777216

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
