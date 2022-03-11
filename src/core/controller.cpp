#include "controller.h"
#include "utils.h"
#include "data_interface.h"

#include <string.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>


Controller::Controller(const char* board_address)
{
    actual_state = controller_state_t::init;

    settings.fps = 30;
    settings.rx_timeout_s = 1;
    settings.key_timeout_millis = 1;
    settings.board_address = board_address;

    nav_info.pitch = 0.0;
    nav_info.roll = 0.0;
    nav_info.yaw = 0.0;
    nav_info.voltage = 0.0;

    throttle_info.system_state = 0;
    throttle_info.throttle_state = 0;
    command_out.forward = true;
    command_out.left = 0;
    command_out.right = 0;
    command_out.throttle_state = 0;

    render_widgets_state = new bool[sizeof(widgets::is_enabled)];
    for(unsigned long i = 0; i < sizeof(render_widgets_state); i++)
    {
        render_widgets_state[i] = video_renderer.get_widget_enabled(i);
    }

    sem_init(&event_sem, 0, 1);

}

void Controller::postEvent(controller_event_t event)
{
    if (actual_state != controller_state_t::closing)
    {
        sem_wait(&event_sem);
        events.push(event);
        sem_post(&event_sem);
    }
}

void Controller::updateRemoteCommand(remote_command_t command, int* args)
{
    switch (command)
    {
    case TURN:
        command_out.left = args[0];
        command_out.right = args[1];
        break;
    case ACCELERATE:
        command_out.throttle_state = args[0] & 0xFF;
        command_out.forward = true;
        break;
    case BREAK:
        command_out.throttle_state = args[0] & 0xFF;
        command_out.forward = false;
        break;
    }
}

Controller::controller_state_t Controller::state()
{
    return actual_state;
}

Controller::navigation_info_t Controller::get_nav_info()
{
    return nav_info;
}

Controller::controller_settings_t Controller::get_settings()
{
    return settings;
}

Controller::throttle_info_t Controller::get_throttle_info()
{
    return throttle_info;
}

void Controller::run()
{

    while (true)
    {
        sem_wait(&event_sem);
        while (!events.empty())
        {
            controller_event_t next_event = events.front();
            events.pop();

            for (auto& entry : state_machine)
            {
                if (entry.actual_state == actual_state && entry.event == next_event)
                {
                    actual_state = entry.next_state;
                    if (entry.action != nullptr)
                    {
                        (this->*entry.action)(entry.action_arg1);
                    }
                    break;
                }
            }
        }
        sem_post(&event_sem);
        printf("run cycle done\n");
    }
}


void Controller::cbit_out(comp_t component, bool component_ok)
{
    cbit_msg msg;
    memset(&msg, 0x00, sizeof(cbit_msg));

    msg.header.msg_id = CBIT_MSG_ID;
    msg.component = component;
    msg.is_failure = !component_ok;
    //send here
}



void Controller::initialize(int arg)
{
    video_renderer.init_window();

    /** I'm ready, send to board start flag **/
    joystick_msg controller_on = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, true, false};

    sendto(throttle_socket, reinterpret_cast<char*>(&controller_on), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&daddr), sizeof(struct sockaddr_in));
}

void Controller::transmit_command(int arg)
{
    joystick_msg controller_cmd;

    controller_cmd.header.msg_id = command_out.forward ? JS_ACC_MSG_ID : JS_BRK_MSG_ID;
    controller_cmd.throttle_state = command_out.throttle_state;
    controller_cmd.x_axis = command_out.left;
    controller_cmd.y_axis = command_out.right;
    controller_cmd.start_flag = false;
    controller_cmd.stop_flag = false;

    sendto(tmp_sock, reinterpret_cast<char*>(&controller_cmd), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&daddr), sizeof(struct sockaddr_in));
    close(tmp_sock);
}

void Controller::update_cbit(int arg)
{

}

void Controller::update_data(int arg)
{
    video_renderer.update_nav_data(nav_info.pitch, nav_info.roll, nav_info.yaw);
    video_renderer.update_remote_system_state(throttle_info.system_state, throttle_info.throttle_state, nav_info.voltage);
    //video_renderer.update_targets(rx.n_targets, rx.data);
}

void Controller::toggle_speed(int arg)
{
    video_renderer.set_widget_enabled(!widgets::is_enabled[widgets::THROTTLESTATE], widgets::THROTTLESTATE);
}

void Controller::toggle_los(int arg)
{
    video_renderer.set_widget_enabled(!widgets::is_enabled[widgets::LOS], widgets::LOS);
}

void Controller::toggle_system_status(int arg)
{
    video_renderer.set_widget_enabled(!widgets::is_enabled[widgets::SYSTEM_STATUS], widgets::SYSTEM_STATUS);
}

void Controller::toggle_videorec(int arg)
{
    video_renderer.toggle_videorec(settings.fps);
}


void Controller::show_help(int arg)
{
    video_renderer.set_widget_enabled(true, widgets::HELP);
}

void Controller::hide_help(int arg)
{
    video_renderer.set_widget_enabled(false, widgets::HELP);
}

void Controller::show_js_help(int arg)
{
    video_renderer.set_widget_enabled(true, widgets::JS_HELP);
}

void Controller::hide_js_help(int arg)
{
    video_renderer.set_widget_enabled(false, widgets::JS_HELP);
}

void Controller::navigate_help(int arg)
{

}

void Controller::enter_devmode(int arg)
{
    for (unsigned long i = 0; i < sizeof(render_widgets_state); i++)
    {
        render_widgets_state[i] = video_renderer.get_widget_enabled(i);
        video_renderer.set_widget_enabled(false, i);
    }

    video_renderer.set_widget_enabled(true, widgets::DEVELOPER_MODE);
}

void Controller::exit_devmode(int arg)
{
    video_renderer.set_widget_enabled(false, widgets::DEVELOPER_MODE);

    for (unsigned long i = 0; i < sizeof(render_widgets_state); i++)
    {
        video_renderer.set_widget_enabled(render_widgets_state[i], i);
    }
}

void Controller::navigate_devmode(int arg)
{
    switch (arg)
    {
    case UP_ARROW:
        video_renderer.devmode_navigate_tab(+1);
        break;
    case DOWN_ARROW:
        video_renderer.devmode_navigate_tab(-1);
        break;
    case LEFT_ARROW:
        video_renderer.devmode_switch_tab(-1);
        break;
    case RIGHT_ARROW:
        video_renderer.devmode_switch_tab(+1);
        break;
    default:
        break;
    }
}

void Controller::halt(int arg)
{
    printf("Requested application exit\n");

    video_renderer.stop();
    for (auto& thread : active_threads)
    {
        thread->join();
    }

    /** I'm exiting, send to board stop flag **/
    joystick_msg controller_off = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, false, true};
    struct sockaddr_in daddr;
    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(THRPORT);
    daddr.sin_addr.s_addr = inet_addr(settings.board_address.c_str());
    sendto(throttle_socket, reinterpret_cast<char*>(&controller_off), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&daddr), sizeof(struct sockaddr_in));

    close(attitude_socket);
    close(throttle_socket);
    close(tgt_socket);
    close(image_socket);
    close(voltage_socket);
    printf("Sockets closed\n");
}
