#include "defs.h"
#include "joystick.h"
#include "keymap.h"

int8_t JoystickInput::map_js_axis_value_int8(int js_axis_value)
{
    int js_axis_span = max_js_axis_value - min_js_axis_value;
    double percentage = ((double) js_axis_value - (double) min_js_axis_value) / (double) js_axis_span;

    return std::numeric_limits<int8_t>::min() + percentage * (std::numeric_limits<int8_t>::max() - std::numeric_limits<int8_t>::min());
}

uint8_t JoystickInput::map_js_axis_value_uint8(int js_axis_value)
{
    int js_axis_span = max_js_axis_value - min_js_axis_value;
    double percentage = ((double) js_axis_value - (double) min_js_axis_value) / (double) js_axis_span;

    return std::numeric_limits<uint8_t>::min() + percentage * (std::numeric_limits<uint8_t>::max() - std::numeric_limits<uint8_t>::min());
}


int JoystickInput::read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));
    if (bytes == sizeof(*event))
        return 0;
    else
        return -1;
}



size_t JoystickInput::get_axis_state(struct js_event *event, struct axis_state axes[3])
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


int JoystickInput::init_joystick()
{
    js = open("/dev/input/js0", O_RDONLY);
    if (js > 0) return 0;
    else if (is_quitting_js) return 0;
    else return -1;
}

void JoystickInput::get_joystick_event()
{
    for (int i = 0; i < 100; i++)
    {
        btn_state[i] = false;
    }

    struct js_event event;
    struct axis_state axes[3];
    memset(axes, 0x00, 3 * sizeof(struct axis_state));

    size_t axis;
    while (!is_quitting_js)
    {
        while (joystick_failure)
        {
            sleep(1);
            joystick_failure = init_joystick() < 0;
        }
        if (read_event(js, &event) == 0)
        {
            switch (event.type)
            {
                case JS_EVENT_BUTTON:
                    btn_state[event.number] = !btn_state[event.number];
                break;
                case JS_EVENT_AXIS:
                    axis = get_axis_state(&event, axes);
                    if (axis == 0)
                    {
                        int* args = new int[2]{map_js_axis_value_int8(axes[axis].x), map_js_axis_value_int8(axes[axis].y)};
                        //controller->updateRemoteCommand(Controller::TURN, args);
                        //controller->postEvent(Controller::controller_event_t::received_joypad);

                        delete[] args;
                    }
                    else if(axis == 1)
                    {
                        int* args = new int(map_js_axis_value_uint8(axes[axis].x));
                        //controller->updateRemoteCommand(Controller::BREAK, args);
                        //controller->postEvent(Controller::controller_event_t::received_joypad);

                        delete args;
                    }
                    else if (event.number == 5)
                    {
                        int* args = new int(map_js_axis_value_uint8(axes[axis].y));
                        //controller->updateRemoteCommand(Controller::ACCELERATE, args);
                        //controller->postEvent(Controller::controller_event_t::received_joypad);

                        delete args;
                    }
                    break;
                default:
                {
                    int* args = new int(0);
                    int* args_turn = new int[2]{0, 0};
                    //controller->updateRemoteCommand(Controller::ACCELERATE, args);
                    //controller->updateRemoteCommand(Controller::TURN, args_turn);
                    //controller->postEvent(Controller::controller_event_t::received_joypad);

                    delete args;
                    delete[] args_turn;
                    break;
                }
            }
        }

        fflush(stdout);
    }

    close(js);

    printf("Joystick thread exit\n");
}

