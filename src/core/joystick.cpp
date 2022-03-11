#include "defs.h"
#include "joystick.h"
#include "keymap.h"

JoystickInput::JoystickInput(DataInterface* iface)
{
    js = nullptr;
    min_js_axis_value = -32767;
    max_js_axis_value = 32767;
    data_iface = iface;
    act_state = IDLE;

    msg_out = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, false, false, false, false, false, false, false};
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


bool JoystickInput::init_joystick()
{
    if (SDL_Init(SDL_INIT_JOYSTICK) == 0)
    {
        SDL_JoystickEventState(SDL_ENABLE);
        js = SDL_JoystickOpen(0);
    }

    return js != nullptr;
}

bool JoystickInput::update_msg_out(SDL_Event event)
{


    bool updated = false;
    switch (event.type)
    {
        case SDL_JOYAXISMOTION:
        {
            if (abs(event.jaxis.value) < JOY_DEAD_CENTER_ZONE)
            {
                return false;
            }
            if (R2_AXIS == event.jaxis.axis)
            {
                msg_out.header.msg_id = JS_ACC_MSG_ID;
                msg_out.throttle_state = map_js_axis_value_uint8(event.jaxis.value);
                updated = true;
            }
            else if (L2_AXIS == event.jaxis.axis)
            {
                msg_out.header.msg_id = JS_BRK_MSG_ID;
                msg_out.throttle_state = map_js_axis_value_uint8(event.jaxis.value);
                updated = true;
            }
            else if (L3_HORIZONTAL_AXIS == event.jaxis.axis)
            {
                msg_out.header.msg_id = JS_ACC_MSG_ID;
                msg_out.x_axis = map_js_axis_value_int8(event.jaxis.value);

                msg_out.y_axis = int8_t(0xFF - uint8_t(msg_out.x_axis));
                updated = true;
            }
            break;
        }
        case  SDL_JOYBUTTONUP:
        {
            if (X_BUTTON == event.jbutton.button)
            {
                emit confirm();
            }
            else if (R1_BUTTON == event.jbutton.button)
            {
                msg_out.right_light_on = !msg_out.right_light_on;
                msg_out.right_light_off = !msg_out.right_light_on;
                updated = true;
            }
            else if (L1_BUTTON == event.jbutton.button)
            {
                msg_out.left_light_on = !msg_out.left_light_on;
                msg_out.left_light_off = !msg_out.left_light_on;
                updated = true;
            }
            else if (PS_BUTTON == event.jbutton.button)
            {
                msg_out.central_light_on = !msg_out.central_light_on;
                msg_out.central_light_off = !msg_out.central_light_on;
                updated = true;
            }

            else (printf("%d\n", event.jbutton.button));

            break;
        }
        case SDL_JOYHATMOTION:
        {
            switch(event.jhat.value)
            {
            case SDL_HAT_LEFTUP:  qDebug("LEFTUP"); break;

            case SDL_HAT_UP: emit up(); break;

            case SDL_HAT_RIGHTUP: qDebug("RIGHTUP"); break;

            case SDL_HAT_LEFT: emit left(); break;

            case SDL_HAT_CENTERED: qDebug("CENTERED"); break;

            case SDL_HAT_RIGHT: emit right(); break;

            case SDL_HAT_LEFTDOWN: qDebug("LEFTDOWN"); break;

            case SDL_HAT_DOWN: emit down(); break;

            case SDL_HAT_RIGHTDOWN: qDebug("RIGHTDOWN"); break;
            }

            break;
        }

        default:
            break;

    }
    return updated;
}

void JoystickInput::call_run()
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
                SDL_Event event;
                if (SDL_NumJoysticks() == 0)
                {
                    msg_out = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, false};
                    data_iface->send_command(remote_stop);
                    act_state = IDLE;
                }
                else
                {
                    while (SDL_PollEvent(&event))
                    {
                        if (update_msg_out(event))
                        {
                            data_iface->send_command(msg_out);
                        }
                    }
                }

            }
            break;
            case EXIT:
            break;
            default:
            break;
        }
    }

    data_iface->send_command(remote_stop);
    printf("Joystick thread exit\n");

    emit thread_quit();
}

void JoystickInput::run()
{
    call_run();
}

