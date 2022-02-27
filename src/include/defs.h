#ifndef DEFS_H
#define DEFS_H

#define APP_NAME "Raspberry PI 4 Car Remote Controller"
#define MAJOR_VERS "01"
#define MINOR_VERS "00"

#define ERR_CREATESOCKDATA "Could not create data socket"
#define ERR_BINDSOCKDATA "Could not bind data socket"
#define ERR_RECVSOCKDATA "Could not receive from data socket"
#define ERR_CREATESOCKVIDEO "Could not create video socket"
#define ERR_BINDSOCKVIDEO "Could not bind video socket"
#define ERR_RECVSOCKVIDEO "Could not receive from video socket"
#define ERR_UNINITIALIZED_SOCKET "Cannot receive: socket uninitialized"
#define ERR_UNINITIALIZED_SOCKET_OUT "Cannot send: socket unitialized"
#define ERR_SENDDATA "Cannot send command"

#define OK_CANSENDDATA "Ready to send data"
#define OK_SENDDATA "Command successfully sent"
#define OK_CREATESOCKDATA "Data server socket was successfully created"
#define OK_BINDSOCKDATA "Data server socket was successfully binded"
#define OK_CREATESOCKVIDEO "Video server socket was successfully created"
#define OK_BINDSOCKVIDEO "Video server socket was successfully binded"
#define OK_CAN_RECEIVE_DATA "Waiting for data"
#define OK_CAN_RECEIVE_VIDEO "Waiting for video"
#define OK_RECVSOCKDATA "Received data from data socket"
#define OK_RECVSOCKVIDEO "Received data from video socket"

#define ERR_UNKNOWN_SOURCE "Received message from unknown source"
#define OK_SPEED "Received SPEED msg"
#define OK_ATTITUDE "Received ATTITUDE msg"
#define OK_THROTTLE "Received THROTTLE STATE msg"
#define OK_TARGET "Received TARGET msg"
#define OK_IMAGE "Received IMAGE msg"

#define DATPORT 8888
#define RENPORT 2222

#define BITPORT 7788
#define BITRESPORT 8877

#define VOLTAGE_MSG_ID   1
#define ATTITUDE_MSG_ID  2
#define ACTUATORS_STATE_MSG_ID  3
#define JS_ACC_MSG_ID           7
#define JS_BRK_MSG_ID           8
#define TARGET_MSG_ID	 9
#define CBIT_MSG_ID     10
#define CBITRES_MSG_ID  11

#define MAX_IMAGESIZE 60000
#define IMAGE_ROWS    650
#define IMAGE_COLS    1200

#define THROTTLE_STATE_MAX 0x7F
#define THROTTLE_EMERGENCY_BREAK 0x70
#define CONTROLLER_ALIVE 0xAD
#define CONTROLLER_DEAD 0xDE

#define MAX_TARGETS 15

#include <stdint.h>

enum class comp_t
{
    TEGRA = 0,
    ATTITUDE = 1,
    VIDEO = 2,
    JOYSTICK = 3,
    ARDUINO = 4
};

typedef struct
{
    uint32_t msg_id;
} msg_header;

typedef struct __attribute__((packed))
{
    uint32_t msg_id;
    float motor_voltage;
} voltage_msg;

typedef struct __attribute__((packed))
{
    msg_header header;
    comp_t component;
    uint8_t is_failure;
} cbit_msg;

typedef struct __attribute__((packed))
{
    msg_header header;
    uint8_t tegra_failure;
    uint8_t att_failure;
    uint8_t vid_failure;
    uint8_t arduino_failure;
    uint8_t js_failure;
} cbit_result_msg;

typedef struct __attribute__((packed))
{
    uint32_t x_pos;
    uint32_t y_pos;
    uint32_t width;
    uint32_t height;
    float confidence;
    char description[100];
} target_data;

typedef struct __attribute__((packed))
{
    msg_header header;
    uint8_t n_targets;
    target_data data[MAX_TARGETS];
} target_msg;

typedef struct
{
    msg_header header;
    double pitch;
    double roll;
    double yaw;
} __attribute__((packed)) attitude_msg;

typedef struct
{
    uint16_t len;
    uint8_t data[MAX_IMAGESIZE];
} __attribute__((packed)) image_msg;

typedef struct __attribute__((packed))
{
    msg_header header;
    uint8_t throttle_state;
    int8_t x_axis;
    int8_t y_axis;
    bool start_flag;
    bool stop_flag;
}  joystick_msg;

typedef struct __attribute__((packed))
{
    msg_header header;
    uint8_t throttle_state;
    uint8_t system_state;
} actuators_state_msg;


#endif //DEFS_H
