#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <queue>
#include <semaphore.h>

#include "video_renderer.h"
#include "keymap.h"


class Controller
{
public:
    typedef void (Controller::*controller_action_t)(int);

    typedef struct
    {
        double pitch;
        double roll;
        double yaw;
        double voltage;
    } navigation_info_t;

    typedef struct
    {
        uint8_t throttle_state;
        uint8_t system_state;
    } throttle_info_t;

    typedef struct
    {
        std::string board_address;
        int fps;
        int rx_timeout_s;
        int key_timeout_millis;
    } controller_settings_t;

    enum controller_state_t
    {
        init,
        operative,
        help,
        js_help,
        devmode,
        closing
    };

    enum controller_event_t
    {
        start_controller,
        key_timeout,
        received_joypad,
        received_toggle_help,
        received_toggle_speed,
        received_toggle_los,
        received_toggle_system_status,
        received_toggle_js_help,
        received_toggle_videorec,
        received_toggle_devmode,
        pressed_up,
        pressed_down,
        pressed_left,
        pressed_right,
        received_data,
        received_cbit,
        stop_controller
    };

    typedef enum
    {
        TURN,
        ACCELERATE,
        BREAK
    } remote_command_t;

    typedef struct
    {
        int8_t     left;
        int8_t     right;
        bool    forward;
        uint8_t throttle_state;
    } command_out_t;

    typedef struct
    {
        controller_state_t actual_state;
        controller_event_t event;
        controller_state_t next_state;
        controller_action_t action;
        int action_arg1;
    }  controller_state_machine_entry_t;

    Controller(const char* board_address);

    void postEvent(controller_event_t event);
    void updateRemoteCommand(remote_command_t, int* args);
    void run();

    controller_state_t state();
    navigation_info_t get_nav_info();
    controller_settings_t get_settings();
    throttle_info_t get_throttle_info();

private:

    controller_state_t actual_state;
    VideoRenderer video_renderer;

    navigation_info_t nav_info;
    controller_settings_t settings;
    throttle_info_t throttle_info;
    command_out_t command_out;

    std::queue<controller_event_t> events;
    std::vector<std::thread*> active_threads;

    bool *render_widgets_state;
    int attitude_socket, throttle_socket, tgt_socket, image_socket, voltage_socket;
    int cbit_tx_sock;
    sem_t event_sem;
    struct sockaddr_in imu_saddr, speed_saddr, attitude_saddr, voltage_saddr, throttle_saddr, tgt_saddr;
    struct sockaddr_in cbit_tx_addr;

    void cbit_out(comp_t component, bool component_ok);

    void attitude_thread();
    void throttle_thread();
    void voltage_thread();
    void target_thread();

    void initialize(int arg);
    void transmit_command(int arg);
    void update_cbit(int arg);
    void update_data(int arg);
    void toggle_speed(int arg);
    void toggle_los(int arg);
    void toggle_system_status(int arg);
    void toggle_videorec(int arg);
    void show_js_help(int arg);
    void hide_js_help(int arg);
    void show_help(int arg);
    void hide_help(int arg);
    void navigate_help(int arg);
    void enter_devmode(int arg);
    void exit_devmode(int arg);
    void navigate_devmode(int arg);
    void halt(int arg);

    std::vector<controller_state_machine_entry_t> state_machine =
    {
        /* Actual state */              /* Event */                                         /* Next state */                /* Action */                        /* Action arguments */
        {controller_state_t::init,      controller_event_t::start_controller,               controller_state_t::operative,  &Controller::initialize,            0},

        {controller_state_t::operative, controller_event_t::received_joypad,                controller_state_t::operative,  &Controller::transmit_command,      0},
        {controller_state_t::operative, controller_event_t::received_cbit,                  controller_state_t::operative,  &Controller::update_cbit,           0},
        {controller_state_t::operative, controller_event_t::received_data,                  controller_state_t::operative,  &Controller::update_data,           0},
        {controller_state_t::operative, controller_event_t::received_toggle_speed,          controller_state_t::operative,  &Controller::toggle_speed,          0},
        {controller_state_t::operative, controller_event_t::received_toggle_los,            controller_state_t::operative,  &Controller::toggle_los,            0},
        {controller_state_t::operative, controller_event_t::received_toggle_system_status,  controller_state_t::operative,  &Controller::toggle_system_status,  0},
        {controller_state_t::operative, controller_event_t::received_toggle_js_help,        controller_state_t::js_help,    &Controller::show_js_help,          0},
        {controller_state_t::operative, controller_event_t::received_toggle_videorec,       controller_state_t::operative,  &Controller::toggle_videorec,       0},
        {controller_state_t::operative, controller_event_t::received_toggle_help,           controller_state_t::help,       &Controller::show_help,             0},
        {controller_state_t::operative, controller_event_t::received_toggle_devmode,        controller_state_t::devmode,    &Controller::enter_devmode,         0},
        {controller_state_t::operative, controller_event_t::stop_controller,                controller_state_t::closing,    &Controller::halt,                  0},
        {controller_state_t::operative, controller_event_t::key_timeout,                    controller_state_t::operative,  nullptr,                            0},

        {controller_state_t::help,      controller_event_t::received_toggle_help,           controller_state_t::operative,  &Controller::hide_help,             0},
        {controller_state_t::help,      controller_event_t::received_joypad,                controller_state_t::help,       &Controller::navigate_help,         0},
        {controller_state_t::help,      controller_event_t::received_cbit,                  controller_state_t::help,       &Controller::update_cbit,           0},
        {controller_state_t::help,      controller_event_t::received_data,                  controller_state_t::help,       &Controller::update_data,           0},
        {controller_state_t::help,      controller_event_t::stop_controller,                controller_state_t::closing,    &Controller::halt,                  0},
        {controller_state_t::help,      controller_event_t::key_timeout,                    controller_state_t::help,       nullptr,                            0},

        {controller_state_t::js_help,   controller_event_t::received_toggle_js_help,        controller_state_t::operative,  &Controller::hide_js_help,          0},
        {controller_state_t::js_help,   controller_event_t::received_joypad,                controller_state_t::js_help,    &Controller::navigate_help,         0},
        {controller_state_t::js_help,   controller_event_t::received_cbit,                  controller_state_t::js_help,    &Controller::update_cbit,           0},
        {controller_state_t::js_help,   controller_event_t::received_data,                  controller_state_t::js_help,    &Controller::update_data,           0},
        {controller_state_t::js_help,   controller_event_t::stop_controller,                controller_state_t::closing,    &Controller::halt,                  0},
        {controller_state_t::js_help,   controller_event_t::key_timeout,                    controller_state_t::js_help,    nullptr,                            0},

        {controller_state_t::devmode,   controller_event_t::received_toggle_devmode,        controller_state_t::operative,  &Controller::exit_devmode,          0},
        {controller_state_t::devmode,   controller_event_t::received_joypad,                controller_state_t::devmode,    &Controller::navigate_devmode,      0},
        {controller_state_t::devmode,   controller_event_t::received_cbit,                  controller_state_t::devmode,    &Controller::update_cbit,           0},
        {controller_state_t::devmode,   controller_event_t::received_data,                  controller_state_t::devmode,    &Controller::update_data,           0},
        {controller_state_t::devmode,   controller_event_t::pressed_up,                     controller_state_t::devmode,    &Controller::navigate_devmode,      UP_ARROW},
        {controller_state_t::devmode,   controller_event_t::pressed_down,                   controller_state_t::devmode,    &Controller::navigate_devmode,      DOWN_ARROW},
        {controller_state_t::devmode,   controller_event_t::pressed_left,                   controller_state_t::devmode,    &Controller::navigate_devmode,      LEFT_ARROW},
        {controller_state_t::devmode,   controller_event_t::pressed_right,                  controller_state_t::devmode,    &Controller::navigate_devmode,      RIGHT_ARROW},
        {controller_state_t::devmode,   controller_event_t::stop_controller,                controller_state_t::closing,    &Controller::halt,                  0},
        {controller_state_t::devmode,   controller_event_t::key_timeout,                    controller_state_t::devmode,    nullptr,                            0}

    };



};

#endif //CONTROLLER_H
