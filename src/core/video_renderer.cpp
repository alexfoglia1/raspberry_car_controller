#include "defs.h"
#include "video_renderer.h"
#include "keymap.h"
#include "widgets.h"
#include "utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <experimental/filesystem>
#include <semaphore.h>
#include <signal.h>

int data_socket, image_socket, cbit_rx_socket;
struct sockaddr_in data_saddr, image_saddr, cbit_tx_addr, cbit_rx_addr;
bool is_recording = false;
bool is_speed_test = false;
bool is_editparams = false;
bool is_quitting_app = false;
bool forward_measurements_to_speedtest = false;
int fps = 30;
int rx_timeout_s = 1;
int key_timeout_millis = 1;
int cbit_tx_sock;
cv::Mat* imagewindow;
cv::VideoWriter* video;
sem_t image_semaphore, tgt_semaphore;

void start_videorec()
{
    int tok = 0;

    char buf[256];
    sprintf(buf, "out_%d.avi", tok);

    while(std::experimental::filesystem::exists((const char*)buf))
    {
        tok += 1;
        sprintf(buf, "out_%d.avi", tok);
    }

    video = new cv::VideoWriter((const char*)buf, cv::VideoWriter::fourcc('M','J','P','G'), fps, imagewindow->size());

    is_recording = true;
}

void stop_videorec()
{
    video->release();
    delete video;

    is_recording = false;
}

void cbit_out(comp_t component, bool component_ok)
{
    cbit_msg msg;
    memset(&msg, 0x00, sizeof(cbit_msg));

    msg.header.msg_id = CBIT_MSG_ID;
    msg.component = component;
    msg.is_failure = !component_ok;

    sendto(cbit_tx_sock, reinterpret_cast<char*>(&msg), sizeof(cbit_msg), 0, reinterpret_cast<struct sockaddr*>(&cbit_tx_addr), sizeof(struct sockaddr_in));
}

void init_localsock(int* localsocket, struct sockaddr_in* localaddr, int port)
{
    *localsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct timeval read_timeout;
    read_timeout.tv_sec = rx_timeout_s;
    read_timeout.tv_usec = 0;
    setsockopt(*localsocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    memset(localaddr, 0x00, sizeof(struct sockaddr_in));
    localaddr->sin_family = AF_INET;
    localaddr->sin_port = htons(port);
    localaddr->sin_addr.s_addr = INADDR_ANY;

    bind(*localsocket, (struct sockaddr*)localaddr, sizeof(struct sockaddr));

}

void cmd_out_task(const char* board_address)
{
    int key = cv::waitKey(key_timeout_millis);
    if(key != -1)
    {
        switch(key)
        {
        case ESCAPE:
            is_quitting_app = true;
            break;
        case TOGGLE_SPEED: /** s keyboard or x joystick **/
            if (is_speed_test)
            {
                bool isStart = forward_measurements_to_speedtest == false;
                bool isStop = forward_measurements_to_speedtest == true;
                forward_measurements_to_speedtest = !forward_measurements_to_speedtest;
                isStart = isStart && forward_measurements_to_speedtest == true;
                isStop = isStop && forward_measurements_to_speedtest == false;

                if (isStart)
                {
                    widgets::devmode::signalStartSpeedTest();
                }
                else if (isStop)
                {
                    widgets::devmode::signalStopSpeedTest();
                }
            }
            else if (is_editparams)
            {
                forward_measurements_to_speedtest = false;
                widgets::devmode::signalSwitchCurrentParam(+1);
            }
            else
            {
                forward_measurements_to_speedtest = false;
                widgets::is_enabled[widgets::THROTTLESTATE] = !widgets::is_enabled[widgets::THROTTLESTATE];
            }
            break;
        case TOGGLE_LOS: /* l keyboard or square joystick */
            if (is_editparams)
            {
                widgets::devmode::editCurrentParamValue(-1);
            }
            else
            {
                widgets::is_enabled[widgets::LOS] = !widgets::is_enabled[widgets::LOS];
            }
            break;
        case TOGGLE_TGT:
            widgets::is_enabled[widgets::TARGETS] = !widgets::is_enabled[widgets::TARGETS];
            break;
        case TOGGLE_MENU:
            widgets::is_enabled[widgets::MENU] = !widgets::is_enabled[widgets::MENU];
            if(widgets::is_enabled[widgets::MENU]) widgets::is_enabled[widgets::JS_MENU] = false;
            break;
        case TOGGLE_SS:  /* y keyboard or circle joystick */
            if (is_editparams)
            {
                widgets::devmode::editCurrentParamValue(+1);
            }
            else
            {
                widgets::is_enabled[widgets::SYSTEM_STATUS] = !widgets::is_enabled[widgets::SYSTEM_STATUS];
            }
            break;
        case TOGGLE_JS:
            widgets::is_enabled[widgets::JS_MENU] = !widgets::is_enabled[widgets::JS_MENU];
            if(widgets::is_enabled[widgets::JS_MENU]) widgets::is_enabled[widgets::MENU] = false;
            break;
        case VIDEOREC:
            if (is_recording)
            {
                stop_videorec();
            }
            else
            {
                start_videorec();
            }
            widgets::is_enabled[widgets::VIDEO_REC] = is_recording;
            break;
        case DEV_MODE:
            widgets::is_enabled[widgets::DEVELOPER_MODE] = !widgets::is_enabled[widgets::DEVELOPER_MODE];
            break;
        case DEV_TAB_RIGHT:
            if (widgets::is_enabled[widgets::DEVELOPER_MODE])
            {
                switch(widgets::devmode::act_tab)
                {
                    case 0: //ATTITUDE TAB
                        widgets::devmode::updateTab(widgets::DEV_VOLTAGE_TAB);
                        break;
                    case 1: //VOLTAGE TAB
                        widgets::devmode::updateTab(widgets::DEV_DETECTOR_TAB);
                        break;
                    case 2: //DEV_DETECTOR_TAB
                        widgets::devmode::updateTab(widgets::DEV_SPEEDTEST_TAB);
                        break;
                    case 3: //DEV_SPEEDTEST_TAB
                        widgets::devmode::updateTab(widgets::DEV_EDITPARAMS_TAB);
                        break;
                    case 4: //DEV_EDITPARAMS_TAB
                        widgets::devmode::updateTab(widgets::DEV_ATTITUDE_TAB);
                        break;
                    default:
                        break;
                }
            }
            break;
        case DEV_TAB_LEFT:
            if (widgets::is_enabled[widgets::DEVELOPER_MODE])
            {
                switch(widgets::devmode::act_tab)
                {
                    case 0: //ATTITUDE TAB
                        widgets::devmode::updateTab(widgets::DEV_EDITPARAMS_TAB);
                        break;
                    case 1: //VOLTAGE TAB
                        widgets::devmode::updateTab(widgets::DEV_ATTITUDE_TAB);
                        break;
                    case 2: //DEV_DETECTOR_TAB
                        widgets::devmode::updateTab(widgets::DEV_VOLTAGE_TAB);
                        break;
                    case 3: //DEV_SPEEDTEST_TAB
                        widgets::devmode::updateTab(widgets::DEV_DETECTOR_TAB);
                        break;
                    case 4: //DEV_EDITPARAMS_TAB
                        widgets::devmode::updateTab(widgets::DEV_SPEEDTEST_TAB);
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
        }
    }
}


void attitude_task(attitude_msg rx, ssize_t bytes_recv)
{
    if(bytes_recv > 0)
    {
        widgets::los::update(rx);
        widgets::devmode::updatePitch((float)rx.pitch);
        widgets::devmode::updateRoll((float)rx.roll);
        widgets::devmode::updateYaw((float)rx.yaw);
    }

    cbit_out(comp_t::ATTITUDE, bytes_recv > 0);
}

void target_task(target_msg rx, ssize_t bytes_recv)
{

    if(bytes_recv > 0)
    {
        sem_wait(&tgt_semaphore);
        widgets::targets::update(rx);
        widgets::devmode::updateTargets(rx.data, rx.n_targets);
        sem_post(&tgt_semaphore);
    }

    cbit_out(comp_t::TEGRA, bytes_recv > 0);
}

void throttle_task(actuators_state_msg rx, ssize_t bytes_recv)
{
    if(bytes_recv > 0)
    {
        widgets::throttlestate::update(rx);
        widgets::systemstatus::updateMotorVoltageOut(rx);
        if (widgets::is_enabled[widgets::DEVELOPER_MODE])
        {
            widgets::devmode::updateVoltageOut(rx.throttle_state / 255.f);
        }
    }

}

void voltage_task(voltage_msg rx, ssize_t bytes_recv)
{
    if(bytes_recv > 0)
    {
        widgets::systemstatus::updateMotorVoltageIn(rx);
        widgets::throttlestate::updateVoltageIn(rx.motor_voltage);

        if (widgets::is_enabled[widgets::DEVELOPER_MODE])
        {
            widgets::devmode::updateVoltageIn(rx.motor_voltage);
        }

        if (forward_measurements_to_speedtest)
        {
            widgets::devmode::updateSpeedTestVoltageIn(rx.motor_voltage);
        }

        cbit_out(comp_t::ARDUINO, true);
    }
    else
    {
        cbit_out(comp_t::ARDUINO, false);
    }
}

void image_task()
{
    if(!widgets::is_enabled[widgets::DEVELOPER_MODE])
    {
        image_msg rx;
        ssize_t bytes_recv = recv(image_socket, &rx, sizeof(image_msg), 0);
        if(bytes_recv > 0)
        {
            std::vector<char> data(rx.data, rx.data + rx.len);
            sem_wait(&image_semaphore);
            *imagewindow = cv::imdecode(cv::Mat(data), 1);
            sem_post(&image_semaphore);
            cbit_out(comp_t::VIDEO, true);
        }
        else
        {
            memset(imagewindow->data, 0x00, imagewindow->dataend - imagewindow->data);
            cbit_out(comp_t::VIDEO, false);
        }
    }
}

void data_task()
{
    char rx[256];
    ssize_t bytes_recv = recv(data_socket, rx, 256, 0);
    msg_header* header = (msg_header*)rx;
    switch (header->msg_id)
    {
    case ATTITUDE_MSG_ID:
        attitude_task(*(attitude_msg*)rx, bytes_recv);
        break;
    case TARGET_MSG_ID:
        target_task(*(target_msg*)rx, bytes_recv);
        break;
    case ACTUATORS_STATE_MSG_ID:
        throttle_task(*(actuators_state_msg*)rx, bytes_recv);
        break;
    case VOLTAGE_MSG_ID:
        voltage_task(*(voltage_msg*)rx, bytes_recv);
        break;
    }
}



void render_window()
{
    cv::Size size = imagewindow->size();
    if (!widgets::is_enabled[widgets::DEVELOPER_MODE])
    {
        sem_wait(&image_semaphore);
        widgets::los::draw(imagewindow, 10U + widgets::los::LOS_RAY, size.height - widgets::los::LOS_RAY -  10U);
        widgets::throttlestate::draw(imagewindow, size.width - 320U, size.height - 30U);
        sem_wait(&tgt_semaphore);
        widgets::targets::draw(imagewindow);
        sem_post(&tgt_semaphore);
        widgets::menu::draw(imagewindow, size.width/2, size.height/2);
        widgets::js_menu::draw(imagewindow, size.width/2, size.height/2);
        widgets::videorec::draw(imagewindow, size.width - 40U, 50U);
        widgets::systemstatus::draw(imagewindow, size.width - 130U, size.height/2);
        sem_post(&image_semaphore);
    }
    else
    {
        sem_wait(&image_semaphore);
        sem_wait(&tgt_semaphore);
        widgets::devmode::draw(imagewindow, size.width/2, size.height/2);
        sem_post(&tgt_semaphore);
        sem_post(&image_semaphore);
    }

    char buf[128];
    sprintf(buf, "%s %s.%s", APP_NAME, MAJOR_VERS, MINOR_VERS);
    cv::imshow(buf, *imagewindow);


    if (is_recording)
    {
        video->write(*imagewindow);
    }
}

void cbit_result_task()
{
    cbit_result_msg rx;
    ssize_t bytes_recv = recv(cbit_rx_socket, &rx, sizeof(cbit_result_msg), 0);
    if(bytes_recv > 0)
    {
        widgets::systemstatus::updateCbit(rx);

        if (rx.tegra_failure)
        {
            target_msg empty;
            memset(&empty, 0x00, sizeof(target_msg));
            widgets::targets::update(empty);
            widgets::devmode::updateTargets(empty.data, 0);
        }
    }
}

void* image_thread(void*)
{
    while (!is_quitting_app) { image_task(); }
    printf("Image receiver thread exit\n");
    return NULL;
}

void* data_thread(void*)
{
    while (!is_quitting_app)
    {
        data_task();
    }
    printf("Attitude receiver thread exit\n");
    return NULL;
}

void* cbit_result_thread(void*)
{
    while (!is_quitting_app) { cbit_result_task(); }
    printf("Cbit result receiver thread exit\n");
    return NULL;
}

void main_loop(const char* board_address)
{
    /** Run threads **/
    pthread_t tid_image, tid_data, tid_cbit;
    pthread_create(&tid_image, NULL, image_thread, NULL);
    pthread_create(&tid_data, NULL, data_thread, NULL);
    pthread_create(&tid_cbit, NULL, cbit_result_thread, NULL);

    while(!is_quitting_app)
    {

        /** Render window task **/
        render_window();

        /** Command out task **/
        cmd_out_task(board_address);

        /** Developer mode changes **/
        is_speed_test = (widgets::is_enabled[widgets::DEVELOPER_MODE]) && (widgets::devmode::act_tab == widgets::DEV_SPEEDTEST_TAB);
        is_editparams = (widgets::is_enabled[widgets::DEVELOPER_MODE]) && (widgets::devmode::act_tab == widgets::DEV_EDITPARAMS_TAB);
        fps = widgets::devmode::fps;
        key_timeout_millis = widgets::devmode::key_timeout_millis;

        if (rx_timeout_s != widgets::devmode::rx_timeout_s)
        {
            close(data_socket);
            close(image_socket);
            close(cbit_rx_socket);

            init_localsock(&data_socket, &data_saddr, DATPORT);
            init_localsock(&image_socket, &image_saddr, RENPORT);
            init_localsock(&cbit_rx_socket, &cbit_rx_addr, BITRESPORT);
        }

        rx_timeout_s = widgets::devmode::rx_timeout_s;

        if (!is_speed_test)
        {
            forward_measurements_to_speedtest = false;
        }
    }

    printf("Requested application exit\n");
    pthread_join(tid_image, NULL);
    pthread_join(tid_data, NULL);
    pthread_join(tid_cbit, NULL);

    /** I'm exiting, send to board stop flag **/
    joystick_msg controller_off;
    memset(&controller_off, 0x00, sizeof(joystick_msg));
    controller_off.header.msg_id = JS_ACC_MSG_ID;
    controller_off.stop_flag = true;
    struct sockaddr_in daddr;
    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(DATPORT);
    daddr.sin_addr.s_addr = inet_addr(board_address);
    sendto(data_socket, reinterpret_cast<char*>(&controller_off), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&daddr), sizeof(struct sockaddr_in));

    close(image_socket);
    close(data_socket);
    close(cbit_rx_socket);
    printf("Sockets closed\n");
}

void init_window(const char* board_address)
{
    imagewindow = new cv::Mat(IMAGE_ROWS, IMAGE_COLS, CV_8UC3, cv::Scalar(0, 0, 0));

    char buf[128];
    sprintf(buf, "%s %s.%s", APP_NAME, MAJOR_VERS, MINOR_VERS);
    cv::imshow(buf, *imagewindow);

    init_localsock(&data_socket, &data_saddr, DATPORT);
    init_localsock(&image_socket, &image_saddr, RENPORT);
    init_localsock(&cbit_rx_socket, &cbit_rx_addr, BITRESPORT);

    cbit_tx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&cbit_tx_addr, 0x00, sizeof(struct sockaddr_in));
    cbit_tx_addr.sin_family = AF_INET;
    cbit_tx_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cbit_tx_addr.sin_port = htons(BITPORT);

    widgets::throttlestate::init();
    widgets::los::init();
    widgets::targets::init();
    widgets::systemstatus::init();
    widgets::devmode::init();

    sem_init(&image_semaphore, 0, 1);
    sem_init(&tgt_semaphore, 0, 1);

    /** I'm ready, send to board start flag **/
    joystick_msg controller_on;
    memset(&controller_on, 0x00, sizeof(joystick_msg));
    controller_on.header.msg_id = JS_ACC_MSG_ID;
    controller_on.start_flag = true;
    struct sockaddr_in daddr;
    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(DATPORT);
    daddr.sin_addr.s_addr = inet_addr(board_address);
    sendto(data_socket, reinterpret_cast<char*>(&controller_on), sizeof(joystick_msg), 0, reinterpret_cast<struct sockaddr*>(&daddr), sizeof(struct sockaddr_in));
}

