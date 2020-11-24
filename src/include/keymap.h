#ifndef KEYMAP_H
#define KEYMAP_H

#include "defs.h"

#define MOVE_FWD 82
#define MOVE_BWD 84
#define MOVE_LFT 81
#define MOVE_RGT 83
#define THR_UP10 85
#define THR_DW10 86
#define THR_ZERO 8
#define THR_MAX  32
#define THR_UP01 56
#define THR_DW01 50

command_msg getCommandOfKey(int key, bool* error);
const char* getNameOfKey(int key);

#endif // KEYMAP_H
