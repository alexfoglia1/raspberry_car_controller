#include "video_renderer.h"
#include <stdio.h>

void print_header(msg_header header)
{
    printf("msg_id: %d\n", header.msg_id);
    printf("sse: %f s\n", (double)header.msg_timestamp.tv_sec + 1e-6 * (double)header.msg_timestamp.tv_usec);
}

void update_imu(imu_msg imu)
{
    print_header(imu.header);
    printf("acc x: %f\n", imu.ax);
    printf("acc y: %f\n", imu.ay);
    printf("acc z: %f\n", imu.az);
    printf("gyro x: %f\n", imu.gyrox);
    printf("gyro y: %f\n", imu.gyroy);
    printf("gyro z: %f\n", imu.gyroz);
    printf("magn x: %f\n", imu.magnx);
    printf("magn y: %f\n", imu.magny);
    printf("magn z: %f\n\n", imu.magnz);
}

void update_speed(speed_msg speed)
{
    print_header(speed.header);
    printf("speed x: %f\n", speed.vx);
    printf("speed y: %f\n", speed.vy);
    printf("speed z: %f\n\n", speed.vz);
}

void update_attitude(attitude_msg attitude)
{
    print_header(attitude.header);
    printf("roll: %f\n", attitude.roll);
    printf("pitch: %f\n", attitude.pitch);
    printf("yaw: %f\n\n", attitude.yaw);
}

void update_radiation(radiation_msg radiation)
{
    print_header(radiation.header);
    printf("CPM: %f\n", radiation.CPM);
    printf("Absorbed dose: %f uSv/h\n\n", radiation.uSv_h);
}
