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
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

#include "defs.h"
#include "joystick.h"
#include "keymap.h"

extern "C" {
    #include <xdo.h>
}

int js;
bool joystick_failure = true;
bool is_quitting_js = false;
void SIGUSR_HANDLER_JS(int)
{
    is_quitting_js = true;
    close(js);
}


const int min_js_axis_value = -32767;
const int max_js_axis_value = 32767;
bool btn_state[100];

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
    else
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



int init_joystick()
{
    js = open("/dev/input/js0", O_RDONLY);
    if (js > 0) return 0;
    else if(is_quitting_js) return 0;
    else return -1;
}

void joystick_task(const char* board_address)
{
    signal(SIGUSR1, SIGUSR_HANDLER_JS);
    for (int i = 0; i < 100; i++)
    {
        btn_state[i] = false;
    }

    struct js_event event;
    struct axis_state axes[3];
    memset(axes, 0x00, 3 * sizeof(struct axis_state));

    size_t axis;
    joystick_msg out;
    cbit_msg joystick_cbit;
    int cbit_sock, board_sock;
    struct sockaddr_in cbit_addr, board_addr;

    memset(&joystick_cbit, 0x00, sizeof(cbit_msg));
    memset(&cbit_addr, 0x00, sizeof(struct sockaddr_in));
    memset(&out, 0x00, sizeof(joystick_msg));

    joystick_cbit.header.msg_id = CBIT_MSG_ID;
    joystick_cbit.component = comp_t::JOYSTICK;

    cbit_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    cbit_addr.sin_family = AF_INET;
    cbit_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cbit_addr.sin_port = htons(BITPORT);

    board_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&board_addr, 0x00, sizeof(struct sockaddr_in));
    board_addr.sin_family = AF_INET;
    board_addr.sin_port = htons(DATPORT);
    board_addr.sin_addr.s_addr = inet_addr(board_address);

    xdo_t * x = xdo_new(":0.0");
    while (!is_quitting_js)
    {
        while(joystick_failure)
        {
            sleep(1);
            joystick_failure = init_joystick() < 0;

            joystick_cbit.is_failure = joystick_failure;
            sendto(cbit_sock, reinterpret_cast<char*>(&joystick_cbit), sizeof(cbit_msg), 0, reinterpret_cast<struct sockaddr*>(&cbit_addr), sizeof(struct sockaddr_in));
            if (!joystick_failure)
            {
                joystick_msg controller_on;
                memset(&controller_on, 0x00, sizeof(joystick_msg));
                controller_on.header.msg_id = JS_ACC_MSG_ID;
                controller_on.start_flag = true;
                sendto(board_sock, reinterpret_cast<char*>(&controller_on), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&board_addr), sizeof(struct sockaddr_in));

            }
        }

        joystick_failure = read_event(js, &event) < 0;
        if (joystick_failure)
        {
            joystick_msg controller_off;
            memset(&controller_off, 0x00, sizeof(joystick_msg));
            controller_off.header.msg_id = JS_ACC_MSG_ID;
            controller_off.stop_flag = true;
            sendto(board_sock, reinterpret_cast<char*>(&controller_off), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&board_addr), sizeof(struct sockaddr_in));
            //xdo_send_keysequence_window(x, CURRENTWINDOW, (const char*)'8',0);
        }
        else
        {
            int keyToSimulate;
            char keySequence[2];
            switch (event.type)
            {
                case JS_EVENT_BUTTON:
                    btn_state[event.number] = !btn_state[event.number];
                    if (btn_state[event.number])
                    {
                        keyToSimulate = jsToKeyboardMap(event.number);
                        if (keyToSimulate != 0)
                        {
                            sprintf(keySequence, "%c", (char)keyToSimulate);
                            xdo_send_keysequence_window(x, CURRENTWINDOW, (const char*)keySequence, 0);
                        }
                    }
                break;
                case JS_EVENT_AXIS:
                    axis = get_axis_state(&event, axes);

                    if (axis == 0)
                    {
                        out.header.msg_id = JS_ACC_MSG_ID;
                        out.x_axis = map_js_axis_value_int8(axes[axis].x);
                        out.y_axis = map_js_axis_value_int8(axes[axis].y);
                        printf("TX to board(%d)\n", out.throttle_state);
                        sendto(board_sock, reinterpret_cast<char*>(&out), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&board_addr), sizeof(struct sockaddr_in));
                    }
                    else if (axis == 1)
                    {
                        out.header.msg_id = JS_BRK_MSG_ID;
                        out.throttle_state = map_js_axis_value_uint8(axes[axis].y);
                        printf("TX to board(%d)\n", out.throttle_state);
                        sendto(board_sock, reinterpret_cast<char*>(&out), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&board_addr), sizeof(struct sockaddr_in));
                    }
                    else if (event.number == 4)
                    {
                        out.header.msg_id = JS_ACC_MSG_ID;
                        out.throttle_state = map_js_axis_value_uint8(axes[axis].x);
                        printf("TX to board(%d)\n", out.throttle_state);
                        sendto(board_sock, reinterpret_cast<char*>(&out), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&board_addr), sizeof(struct sockaddr_in));
                    }
                    break;
                default:
                    /* Ignore init events. */
                    break;
            }

        }

        fflush(stdout);
    }

    close(js);

    printf("Joystick sub-process exit\n");
}

