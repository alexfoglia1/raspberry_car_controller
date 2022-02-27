#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <stdint.h>
#include <limits>
#include <string.h>
#include <QThread>
#include <signal.h>

#include "data_interface.h"

class JoystickInput : public QThread
{
    Q_OBJECT
public:
    const joystick_msg remote_start = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, true, false};
    const joystick_msg remote_stop =  {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, true};
    struct axis_state
    {
        short x, y;
    };

    enum js_thread_state_t
    {
        IDLE,
        OPERATIVE,
        EXIT
    };

    JoystickInput(DataInterface* iface);
    void call_run();

public slots:
    void stop();

signals:
    void js_failure();
    void js_on();
    void thread_quit();

protected:
    void run() override;

private:
    int js;
    int min_js_axis_value;
    int max_js_axis_value;
    DataInterface* data_iface;
    js_thread_state_t act_state;
    joystick_msg msg_out;

    bool update_msg_out(struct js_event event, struct axis_state axes[3]);
    bool read_event(int fd, struct js_event *event);
    bool init_joystick();
    int8_t map_js_axis_value_int8(int js_axis_value);
    uint8_t map_js_axis_value_uint8(int js_axis_value);
    size_t get_axis_state(struct js_event *event, struct axis_state axes[3]);
};


#endif //JOYSTICK_H
