#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <stdint.h>
#include <limits>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

extern "C" {
    #include <xdo.h>
}

class JoystickInput
{
public:
    struct axis_state
    {
        short x, y;
    };


    JoystickInput()
    {
        js = -1;
        joystick_failure = true;
        is_quitting_js = false;
        min_js_axis_value = -32767;
        max_js_axis_value = 32767;
    }

    void get_joystick_event();

private:
    int js;
    bool joystick_failure;
    bool is_quitting_js;
    int min_js_axis_value;
    int max_js_axis_value;
    bool btn_state[100];

    int8_t map_js_axis_value_int8(int js_axis_value);
    uint8_t map_js_axis_value_uint8(int js_axis_value);
    int read_event(int fd, struct js_event *event);
    size_t get_axis_state(struct js_event *event, struct axis_state axes[3]);
    int init_joystick();
};


#endif //JOYSTICK_H
