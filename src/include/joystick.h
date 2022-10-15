#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <limits>
#include <string.h>
#include <QTimer>
#include <QThread>
#include <signal.h>
#include <SDL.h>
#include <semaphore.h>

#include "data_interface.h"

class JoystickInput : public QThread
{
    Q_OBJECT
public:
    const joystick_msg remote_start = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, true, false};
    const joystick_msg remote_stop =  {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, true};
    const int R2_AXIS = 5;
    const int L2_AXIS = 2;
    const int L2_BUTTON = 6;
    const int R2_BUTTON = 7;
    const int X_BUTTON = 1;
    const int R1_BUTTON = 5;
    const int L1_BUTTON = 4;
    const int PS_BUTTON = 12;
    const int L3_HORIZONTAL_AXIS = 0;
    const int JOY_DEAD_CENTER_ZONE = 3000;
    enum js_thread_state_t
    {
        IDLE,
        OPERATIVE,
        EXIT
    };

    JoystickInput(DataInterface* iface);

public slots:
    void quit();
    void on_toggle_timed_acceleration();

private slots:
    void on_accelerator_timeout();
    void on_decelerator_timeout();

signals:
    void js_failure();
    void js_on();
    void right();
    void left();
    void up();
    void down();
    void confirm();
    void thread_quit();
    void toggle_timed_acceleration_result(bool);
protected:
    void run() override;

private:
    SDL_Joystick *js;

    int min_js_axis_value;
    int max_js_axis_value;
    DataInterface* data_iface;
    js_thread_state_t act_state;
    joystick_msg msg_out;
    bool timed_acceleration;
    sem_t timed_acceleration_semaphore;
    sem_t msg_out_semaphore;
    double first_l2_buttondown;
    double first_r2_buttondown;
    QTimer timed_accelerator;
    QTimer timed_decelerator;

    bool update_msg_out(SDL_Event event);
    bool init_joystick();
    int8_t map_js_axis_value_int8(int js_axis_value);
    uint8_t map_js_axis_value_uint8(int js_axis_value);
    uint8_t fun_timed_acceleration(double t);

};


#endif //JOYSTICK_H
