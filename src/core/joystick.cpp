#include "defs.h"
#include "joystick.h"
#include "keymap.h"

#include <QDateTime>
#include <QApplication>

JoystickInput::JoystickInput(DataInterface* iface)
{
    js = nullptr;
    min_js_axis_value = -32767;
    max_js_axis_value = 32767;
    data_iface = iface;
    act_state = IDLE;
    timed_acceleration = false;
    first_r2_buttondown = -1;
    first_l2_buttondown = -1;
    sem_init(&timed_acceleration_semaphore, 0, 1);
    sem_init(&msg_out_semaphore, 0, 1);
    timed_accelerator.setInterval(20);
    timed_decelerator.setInterval(20);
    connect(&timed_accelerator, SIGNAL(timeout()), this, SLOT(on_accelerator_timeout()));
    connect(&timed_decelerator, SIGNAL(timeout()), this, SLOT(on_decelerator_timeout()));
    msg_out = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, false, false, false, false, false, false, false};
}

void JoystickInput::quit()
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
    double act_millis = 0;

    switch (event.type)
    {
        case SDL_JOYAXISMOTION:
        {
            if (abs(event.jaxis.value) < JOY_DEAD_CENTER_ZONE)
            {
                return false;
            }
            if (R2_AXIS == event.jaxis.axis && !timed_acceleration)
            {
                msg_out.header.msg_id = JS_ACC_MSG_ID;
                msg_out.throttle_state = map_js_axis_value_uint8(event.jaxis.value);
                updated = true;
            }
            else if (L2_AXIS == event.jaxis.axis && !timed_acceleration)
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
        case SDL_JOYBUTTONDOWN:
            act_millis = QDateTime::currentMSecsSinceEpoch();

            sem_wait(&timed_acceleration_semaphore);

            if (R2_BUTTON == event.jbutton.button && timed_acceleration)
            {
                first_r2_buttondown = act_millis;
            }
            else if (L2_BUTTON == event.jbutton.button && timed_acceleration)
            {
                first_l2_buttondown = act_millis;
            }

            sem_post(&timed_acceleration_semaphore);
            break;
        case  SDL_JOYBUTTONUP:
        {
            sem_wait(&timed_acceleration_semaphore);

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
            else if (R2_BUTTON == event.jbutton.button && timed_acceleration)
            {
                msg_out.header.msg_id = JS_ACC_MSG_ID;
                msg_out.throttle_state = 0;
                first_r2_buttondown = -1;
                updated = true;
            }
            else if (L2_BUTTON == event.jbutton.button && timed_acceleration)
            {
                msg_out.header.msg_id = JS_BRK_MSG_ID;
                msg_out.throttle_state = 0;
                first_l2_buttondown = -1;
                updated = true;
            }

            sem_post(&timed_acceleration_semaphore);
            break;
        }
        case SDL_JOYHATMOTION:
        {
            switch(event.jhat.value)
            {

            case SDL_HAT_UP: emit up(); break;

            case SDL_HAT_LEFT: emit left(); break;

            case SDL_HAT_RIGHT: emit right(); break;

            case SDL_HAT_DOWN: emit down(); break;
            }

            break;
        }

        default:
            break;

    }

    return updated;
}

void JoystickInput::on_toggle_timed_acceleration()
{
    sem_wait(&timed_acceleration_semaphore);

    timed_acceleration = !timed_acceleration;
    if (timed_acceleration)
    {
        timed_accelerator.start();
        timed_decelerator.start();
    }
    else
    {
        timed_accelerator.stop();
        timed_decelerator.stop();
    }
    emit toggle_timed_acceleration_result(timed_acceleration);

    sem_post(&timed_acceleration_semaphore);
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
                SDL_Event event;
                if (SDL_NumJoysticks() == 0)
                {
                    msg_out = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, false, false, false, false, false, false, false};
                    data_iface->send_command(remote_stop);
                    act_state = IDLE;
                }
                else
                {
                    sem_wait(&msg_out_semaphore);
                    while (SDL_PollEvent(&event))
                    {
                        if (update_msg_out(event))
                        {
                            data_iface->send_command(msg_out);
                        }
                    }
                    sem_post(&msg_out_semaphore);
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

uint8_t JoystickInput::fun_timed_acceleration(double t)
{
    double dt_s = t/1000.0;
    const double Q = 0;
    const double M = 750 * dt_s + Q;
    const double f = M*dt_s + Q;
    if (f >= 255)
    {
        return uint8_t(0xFF);
    }
    else
    {
        return uint8_t(f);
    }
}

void JoystickInput::on_accelerator_timeout()
{
    if (first_r2_buttondown < 0) return;
    double dt_millis = QDateTime::currentMSecsSinceEpoch() - first_r2_buttondown;
    sem_wait(&msg_out_semaphore);
    msg_out.header.msg_id = JS_ACC_MSG_ID;
    msg_out.throttle_state = fun_timed_acceleration(dt_millis);
    data_iface->send_command(msg_out);

    sem_post(&msg_out_semaphore);
}

void JoystickInput::on_decelerator_timeout()
{
    if (first_l2_buttondown < 0) return;

    double dt_millis = QDateTime::currentMSecsSinceEpoch() - first_l2_buttondown;
    sem_wait(&msg_out_semaphore);
    msg_out.header.msg_id = JS_BRK_MSG_ID;
    msg_out.throttle_state = fun_timed_acceleration(dt_millis);
    data_iface->send_command(msg_out);

    sem_post(&msg_out_semaphore);
}
