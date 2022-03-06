#ifndef DEFS_H
#define DEFS_H

#include <opencv2/opencv.hpp>

#define APP_NAME    "Raspberry PI 4 Car Remote Controller"
#define MAJOR_VERS  "02"
#define MINOR_VERS  "00"

#define UDP_PORT                8888
#define UDP_PORT_VIDEO          2222

#define VOLTAGE_MSG_ID          1
#define ATTITUDE_MSG_ID         2
#define ACTUATORS_STATE_MSG_ID  3
#define JS_ACC_MSG_ID           7
#define JS_BRK_MSG_ID           8
#define TARGET_MSG_ID           9

#define MAX_IMAGESIZE 60000
#define IMAGE_ROWS    650
#define IMAGE_COLS    1200

#define MAX_TARGETS 15

#include <stdint.h>
#include <QObject>

enum class comp_t
{
    TEGRA = 0,
    ATTITUDE = 1,
    VIDEO = 2,
    JOYSTICK = 3,
    ARDUINO = 4,
    MOTORS = 5
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
    bool left_light_on;
    bool left_light_off;
    bool right_light_on;
    bool right_light_off;
    bool central_light_on;
    bool central_light_off;
}  joystick_msg;

typedef struct __attribute__((packed))
{
    msg_header header;
    uint8_t throttle_state;
    uint8_t system_state;
} actuators_state_msg;

Q_DECLARE_METATYPE(attitude_msg)
Q_DECLARE_METATYPE(target_msg)
Q_DECLARE_METATYPE(actuators_state_msg)
Q_DECLARE_METATYPE(joystick_msg)
Q_DECLARE_METATYPE(voltage_msg)
Q_DECLARE_METATYPE(image_msg)
Q_DECLARE_METATYPE(comp_t)
Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(cv::Rect)


#endif //DEFS_H
