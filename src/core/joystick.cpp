#include "defs.h"
#include "joystick.h"
#include "keymap.h"

JoystickInput::JoystickInput(DataInterface* iface)
{
    js = -1;
    min_js_axis_value = -32767;
    max_js_axis_value = 32767;
    data_iface = iface;
    act_state = IDLE;

    msg_out = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, false};
}

void JoystickInput::stop()
{
    act_state = EXIT;
}

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


bool JoystickInput::read_event(int fd, struct js_event *event)
{
    return read(fd, event, sizeof(*event)) == sizeof(*event);
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


bool JoystickInput::init_joystick()
{
    js = open("/dev/input/js0", O_RDONLY);
    return js > 0;
}

void JoystickInput::update_msg_out(js_event event, axis_state axes[3])
{
    switch (event.type)
    {
        case JS_EVENT_AXIS:
        {
            size_t axis = get_axis_state(&event, axes);
            if (axis == 0)
            {
                msg_out.header.msg_id = JS_ACC_MSG_ID;
                msg_out.x_axis = map_js_axis_value_int8(axes[axis].x);
                msg_out.y_axis = map_js_axis_value_int8(axes[axis].y);
            }
            else if (axis == 1)
            {
                msg_out.header.msg_id = JS_BRK_MSG_ID;
                msg_out.throttle_state = axes[axis].y;
            }
            else if (event.number == 4)
            {
                msg_out.header.msg_id = JS_ACC_MSG_ID;
                msg_out.throttle_state = axes[axis].x;
            }
        }
        break;
        default:
        break;

    }
}

void JoystickInput::run()
{
    while (act_state != EXIT)
    {
        switch (act_state)
        {
            case IDLE:
            {
                if (init_joystick())
                {
                    data_iface->send_command(remote_start);
                    act_state = OPERATIVE;
                    emit js_on();
                }
                else
                {
                    act_state = IDLE;
                    emit js_failure();
                }
            }
            break;
            case OPERATIVE:
            {
                struct js_event event;
                struct axis_state axes[3];
                if (read_event(js, &event))
                {
                    update_msg_out(event, axes);
                    data_iface->send_command(msg_out);
                }
                else
                {
                    msg_out = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, false};
                    data_iface->send_command(remote_stop);
                    act_state = IDLE;
                }
            }
            break;
            case EXIT:
            break;
            default:
            break;
        }
    }

    close(js);
    data_iface->send_command(remote_stop);
    printf("Joystick thread exit\n");

    emit thread_quit();
}

