#ifndef DEFS_H
#define DEFS_H

#define PROJNAME "Raspberry PI 4 Car Remote Controller v01.00"

#define ERR_CREATESOCKDATA "Could not create data socket"
#define ERR_BINDSOCKDATA "Could not bind data socket"
#define ERR_RECVSOCKDATA "Could not receive from data socket"
#define ERR_CREATESOCKVIDEO "Could not create video socket"
#define ERR_BINDSOCKVIDEO "Could not bind video socket"
#define ERR_RECVSOCKVIDEO "Could not receive from video socket"
#define ERR_UNINITIALIZED_SOCKET "Cannot receive: socket uninitialized"

#define OK_CREATESOCKDATA "Data server socket was successfully created"
#define OK_BINDSOCKDATA "Data server socket was successfully binded"
#define OK_CREATESOCKVIDEO "Video server socket was successfully created"
#define OK_BINDSOCKVIDEO "Video server socket was successfully binded"
#define OK_CAN_RECEIVE_DATA "Waiting for data"
#define OK_CAN_RECEIVE_VIDEO "Waiting for video"
#define OK_RECVSOCKDATA "Received data from data socket"
#define OK_RECVSOCKVIDEO "Received data from video socket"

#define ERR_UNKNOWN_SOURCE "Received message from unknown source"
#define OK_IMU "Received IMU msg"
#define OK_SPEED "Received SPEED msg"
#define OK_ATTITUDE "Received ATTITUDE msg"
#define OK_RADIATION "Received RADIATION msg"
#define OK_IMAGE "Received IMAGE msg"

#define DATPORT 1234
#define VIDPORT 4321
#define IMUPORT 7777
#define VELPORT 8888
#define ATTPORT 9999
#define RADPORT 3636
#define RENPORT 4444

#define BOARD_ADDRESS "192.168.1.123"

#define IMU_MSG_ID       0
#define SPEED_MSG_ID     1
#define ATTITUDE_MSG_ID  2
#define RADIATION_MSG_ID 3

#define SCREEN_ROWS 255
#define SCREEN_COLS 255

#include <stdint.h>
#include <sys/time.h>


typedef struct timeval timeval;
typedef struct
{
    timeval  msg_timestamp;
    uint32_t msg_id;
} msg_header;

typedef struct
{
    msg_header header;
    double ax;
    double ay;
    double az;
    double gyrox;
    double gyroy;
    double gyroz;
    double magnx;
    double magny;
    double magnz;
} imu_msg;

typedef struct
{
    msg_header header;
    double vx;
    double vy;
    double vz;
} speed_msg;

typedef struct
{
    msg_header header;
    double pitch;
    double roll;
    double yaw;
} attitude_msg;

typedef struct
{
    msg_header header;
    double CPM;
    double uSv_h;
} radiation_msg;

typedef struct
{
    uint8_t image[SCREEN_ROWS][SCREEN_COLS][3];
} image_msg;

#endif //DEFS_H
