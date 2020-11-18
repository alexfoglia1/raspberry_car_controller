#ifndef VIDEO_RECEIVER_H
#define VIDEO_RECEIVER_H
#include <stdbool.h>

bool init_videoserver();
bool recv_video_from_board(char* msg);

#endif //VIDEO_RECEIVER_H
