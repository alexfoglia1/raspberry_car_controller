#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H
#include "defs.h"

void update_imu(imu_msg imu);
void update_speed(speed_msg speed);
void update_attitude(attitude_msg attitude);
void update_radiation(radiation_msg radiation);

#endif //VIDEO_RENDERER_H
