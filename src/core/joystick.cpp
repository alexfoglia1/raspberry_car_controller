/**
 * Author: Jason White
 *
 * Description:
 * Reads joystick/gamepad events and displays them.
 *
 * Compile:
 * gcc joystick.c -o joystick
 *
 * Run:
 * ./joystick [/dev/input/jsX]
 *
 * See also:
 * https://www.kernel.org/doc/Documentation/input/joystick-api.txt
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <stdint.h>
#include <limits>
#include <string.h>

#include "defs.h"
#include "data_transceiver.h"

const int min_js_axis_value = -32767;
const int max_js_axis_value = 32767;

int8_t map_js_axis_value_int8(int js_axis_value)
{
    int js_axis_span = max_js_axis_value - min_js_axis_value;
    double percentage = ((double) js_axis_value - (double) min_js_axis_value) / (double) js_axis_span;

    return std::numeric_limits<int8_t>::min() + percentage * (std::numeric_limits<int8_t>::max() - std::numeric_limits<int8_t>::min());
}

uint8_t map_js_axis_value_uint8(int js_axis_value)
{
    int js_axis_span = max_js_axis_value - min_js_axis_value;
    double percentage = ((double) js_axis_value - (double) min_js_axis_value) / (double) js_axis_span;

    return std::numeric_limits<uint8_t>::min() + percentage * (std::numeric_limits<uint8_t>::max() - std::numeric_limits<uint8_t>::min());
}

static int js;

/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
    __u8 axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}

/**
 * Current state of an axis.
 */
struct axis_state {
    short x, y;
};

/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
    size_t axis = event->number / 2;

    if (axis < 3)
    {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}

bool init_joystick()
{
    js = open("/dev/input/js0", O_RDONLY);
    printf("open joystick: %d\n", js);
    return js > 0;
}

int joystick_task(const char* board_address)
{
    init_joystick();

    struct js_event event;
    struct axis_state axes[3];
    memset(axes, 0x00, 3 * sizeof(struct axis_state));

    size_t axis;
    joystick_xy_msg out_xy;
    joystick_throttle_msg out_throttle;
    joystick_break_msg out_break;
    memset(&out_xy, 0x00, sizeof(joystick_xy_msg));
    memset(&out_throttle, 0x00, sizeof(joystick_throttle_msg));
    memset(&out_break, 0x00, sizeof(joystick_break_msg));
    out_break.header.msg_id = JS_BR_MSG_ID;
    out_xy.header.msg_id = JS_XY_MSG_ID;
    out_throttle.header.msg_id = JS_TH_MSG_ID;

    /* This loop will exit if the controller is unplugged. */
    while (read_event(js, &event) == 0)
    {
        switch (event.type)
        {
            case JS_EVENT_BUTTON:
                break;

            case JS_EVENT_AXIS:
                axis = get_axis_state(&event, axes);

                if (axis == 0)
                {
                    printf("Axis %ld at (%d, %d)\n", axis, map_js_axis_value_int8(axes[axis].x), map_js_axis_value_int8(axes[axis].y));
                    out_xy.x_axis = map_js_axis_value_int8(axes[axis].x);
                    out_xy.y_axis = map_js_axis_value_int8(axes[axis].y);
                    send_data_to_board(reinterpret_cast<char*>(&out_xy), board_address);
                }
                else if(axis == 1)
                {
                    printf("Axis %ld at (%d, %d)\n", axis, map_js_axis_value_uint8(axes[axis].x), map_js_axis_value_uint8(axes[axis].y));
                    out_break.backward = map_js_axis_value_uint8(axes[axis].x);
                    printf("sent backward(%d)\n", out_break.backward);
                    send_data_to_board(reinterpret_cast<char*>(&out_break), board_address);
                }
                else if(axis != 0 && axis != 1 && event.number == 5)
                {
                    printf("Axis %ld at (%d, %d)\n", axis, map_js_axis_value_uint8(axes[axis].x), map_js_axis_value_uint8(axes[axis].y));
                    out_throttle.throttle_state = map_js_axis_value_uint8(axes[axis].y);
                    send_data_to_board(reinterpret_cast<char*>(&out_throttle), board_address);
                }
                break;
            default:
                /* Ignore init events. */
                break;
        }
        
        fflush(stdout);
    }

    close(js);
    return 0;
}

