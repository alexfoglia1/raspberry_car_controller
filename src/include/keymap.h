#ifndef KEYMAP_H
#define KEYMAP_H

#include "defs.h"

#define MOVE_FWD (int)'w'
#define MOVE_BWD (int)'x'
#define MOVE_LFT (int)'a'
#define MOVE_RGT (int)'d'
#define THR_UP10 (int)'p'
#define THR_DW10 (int)'q'
#define THR_ZERO 8
#define THR_MAX  32
#define THR_UP01 56
#define THR_DW01 50

command_msg getCommandOfKey(int key, bool* error);
const char* getNameOfKey(int key);

#endif // KEYMAP_H
