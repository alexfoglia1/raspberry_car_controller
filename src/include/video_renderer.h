#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H
#include "defs.h"

void update_imu(imu_msg imu);
void update_speed(speed_msg speed);
void update_attitude(attitude_msg attitude);
void update_radiation(radiation_msg radiation);
void update_image(image_msg image);

void init_window(int argc, char** argv);
void show_window();
#endif //VIDEO_RENDERER_H
