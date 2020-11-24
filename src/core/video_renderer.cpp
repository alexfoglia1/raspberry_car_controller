#include "defs.h"
#include "video_renderer.h"
#include "data_transceiver.h"
#include "keymap.h"

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#define ACC_POS 13, SCREEN_ROWS - 13
#define GYRO_POS 13, SCREEN_ROWS - 26
#define MAGN_POS 13, SCREEN_ROWS - 39
#define SPEED_POS 5 * SCREEN_COLS/7, SCREEN_ROWS - 13
#define ATT_POS 13, 13
#define RAD_POS 5 * SCREEN_COLS/7, 13
#define VIDEO_POS SCREEN_COLS/4, SCREEN_ROWS/6

char acc_display[256];
char gyro_display[256];
char magn_display[256];
char speed_display[256];
char roll_display[256];
char pitch_display[256];
char yaw_display[256];
char rad_display[256];
char cmd_display[256];
char throttle_display[256];

int imu_socket, speed_socket, attitude_socket, radiation_socket, image_socket;
struct sockaddr_in imu_saddr, speed_saddr, attitude_saddr, radiation_saddr, image_saddr;
cv::Mat* imagewindow;

void init_localsock(int* localsocket, struct sockaddr_in* localaddr, int port)
{
    *localsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(*localsocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    memset(localaddr, 0x00, sizeof(struct sockaddr_in));
    localaddr->sin_family = AF_INET;
    localaddr->sin_port = htons(port);
    localaddr->sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(*localsocket, (struct sockaddr*)localaddr, sizeof(struct sockaddr));
}

void update_image(image_msg image)
{
    std::vector<char> data(image.data, image.data + image.len);
    *imagewindow = cv::imdecode(cv::Mat(data), 1);
}

void update_imu(imu_msg imu)
{
    sprintf(acc_display,  "Acc. : %f %f %f", imu.ax, imu.ay, imu.az);
    sprintf(gyro_display, "Gyro.: %f %f %f", imu.gyrox, imu.gyroy, imu.gyroz);
    sprintf(magn_display, "Magn.: %f %f %f", imu.magnx, imu.magny, imu.magnz);
}

void update_speed(speed_msg speed)
{
    sprintf(speed_display, "Speed: %f %f %f", speed.vx, speed.vy, speed.vz);
}

void update_attitude(attitude_msg attitude)
{
    sprintf(roll_display, "Roll: %f", attitude.roll);
    sprintf(pitch_display, "Pitch: %f", attitude.pitch);
    sprintf(yaw_display, "Yaw: %f", attitude.yaw);
}

void update_radiation(radiation_msg radiation)
{
    sprintf(rad_display, "CPM: %f uSv/h: %f", radiation.CPM, radiation.uSv_h);
}

void update_throttle(uint8_t th_state)
{
    char th_progress[100];
    double perc = ((double)th_state / 255.0) * 100.0;

    for(uint8_t i = 0; i < 100; i++)
    {
        th_progress[i] = (i < perc) ? '*' : ' ';
    }

    sprintf(throttle_display, "[%s]", th_progress);
}

void render_window()
{
    cv::Size size = imagewindow->size();

    std::string text_acc(acc_display);
    std::string text_gyro(gyro_display);
    std::string text_magn(magn_display);
    std::string text_speed(speed_display);
    std::string text_roll(roll_display);
    std::string text_pitch(pitch_display);
    std::string text_yaw(yaw_display);
    std::string text_radiation(rad_display);
    std::string text_command(cmd_display);
    std::string text_throttle(throttle_display);

    cv::putText(*imagewindow, text_acc, cv::Point2d(2U, 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_gyro, cv::Point2d(2U, 40U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_magn, cv::Point2d(2U, 60U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_speed, cv::Point2d(2U, 80U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_roll, cv::Point2d(2U, size.height/2 - 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_pitch, cv::Point2d(2U, size.height/2), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_yaw, cv::Point2d(2U, size.height/2 + 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_radiation, cv::Point2d(2U, size.height - 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_command, cv::Point2d(size.width - 120U, 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_4);
    cv::putText(*imagewindow, text_throttle, cv::Point2d(size.width/2, size.height - 20U), cv::FONT_HERSHEY_SIMPLEX, 0.175, cv::Scalar(255,0,0), 1, cv::LINE_4);

    cv::imshow(PROJNAME, *imagewindow);
}

static uint8_t th_state;

void cmd_out_task()
{
    int key = cv::waitKey(1);
    if(key != -1)
    {
        bool error;
        command_msg cmd_out = getCommandOfKey(key, &error);
        sprintf(cmd_display, "OUT: %s", getNameOfKey(key));
        if(!error)
        {
            gettimeofday(&cmd_out.header.msg_timestamp, nullptr);
            send_data_to_board(reinterpret_cast<char*>(&cmd_out));
            th_state = cmd_out.throttle_add == 0x70 ? 0:
                       cmd_out.throttle_add == 0x7F ? 0xFF:
                       th_state + cmd_out.throttle_add;
            update_throttle(th_state); //TODO: remove th_state and receive it remotely
        }
    }
}


void imu_task()
{
    imu_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(imu_socket, &recv, sizeof(imu_msg), 0, (struct sockaddr*)&imu_saddr, &len);
    if(bytes_recv > 0)
    {
        update_imu(recv);
    }
}

void image_task()
{
    memset(imagewindow->data, 0x00, imagewindow->dataend - imagewindow->data);

    image_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(image_socket, &recv, sizeof(image_msg), 0, (struct sockaddr*)&image_saddr, &len);
    if(bytes_recv > 0)
    {
        update_image(recv);
    }
}

void speed_task()
{
    speed_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(speed_socket, &recv, sizeof(speed_msg), 0, (struct sockaddr*)&speed_saddr, &len);
    if(bytes_recv > 0)
    {
        update_speed(recv);
    }
}

void attitude_task()
{
    attitude_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(attitude_socket, &recv, sizeof(attitude_msg), 0, (struct sockaddr*)&attitude_saddr, &len);
    if(bytes_recv > 0)
    {
        update_attitude(recv);
    }
}

void radiation_task()
{
    radiation_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(radiation_socket, &recv, sizeof(radiation_msg), 0, (struct sockaddr*)&radiation_saddr, &len);
    if(bytes_recv > 0)
    {
        update_radiation(recv);
    }
}

void throttle_task()
{
    //recv_th_state
    //update_throttle(rand()&0xFF);
}

void main_loop()
{
    while(true)
    {
        imu_task();
        speed_task();
        attitude_task();
        radiation_task();

        cmd_out_task();
        throttle_task();

        image_task();
        render_window();
    }
}

void init_window()
{

    init_localsock(&imu_socket, &imu_saddr, IMUPORT);
    init_localsock(&speed_socket, &speed_saddr, VELPORT);
    init_localsock(&attitude_socket, &attitude_saddr, ATTPORT);
    init_localsock(&radiation_socket, &radiation_saddr, RADPORT);
    init_localsock(&image_socket, &image_saddr, RENPORT);

    sprintf(acc_display,   "Acc. : %f %f %f", 0., 0., 0.);
    sprintf(gyro_display,  "Gyro.: %f %f %f", 0., 0., 0.);
    sprintf(magn_display,  "Magn.: %f %f %f", 0., 0., 0.);
    sprintf(speed_display, "Speed: %f %f %f", 0., 0., 0.);
    sprintf(roll_display, "Roll: %f", 0.);
    sprintf(pitch_display, "Pitch: %f", 0.);
    sprintf(yaw_display, "Yaw: %f", 0.);
    sprintf(rad_display,   "CPM: %f uSv/h: %f", 0., 0.);
    sprintf(cmd_display, "OUT: NONE");
    sprintf(throttle_display, "[                                                                                                    ]");
    imagewindow = new cv::Mat(600, 600, CV_8UC3, cv::Scalar(0, 0, 0));

    cv::imshow(PROJNAME, *imagewindow);
}

