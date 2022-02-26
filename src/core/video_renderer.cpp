#include "defs.h"
#include "video_renderer.h"
#include "keymap.h"
#include "widgets.h"
#include "utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <experimental/filesystem>
#include <semaphore.h>
#include <signal.h>

VideoRenderer::VideoRenderer()
{
    stopped = false;
    save_frame = false;

    sem_init(&image_semaphore, 0, 1);
}

void VideoRenderer::init_window()
{
    imagewindow = new cv::Mat(IMAGE_ROWS, IMAGE_COLS, CV_8UC3, cv::Scalar(0, 0, 0));

    cv::imshow(PROJNAME, *imagewindow);

    widgets::throttlestate::init();
    widgets::los::init();
    widgets::targets::init();
    widgets::systemstatus::init();
    //widgets::devmode::init();
}

void VideoRenderer::update(image_msg image)
{
    std::vector<char> data(image.data, image.data + image.len);

    sem_wait(&image_semaphore);
    *imagewindow = cv::imdecode(cv::Mat(data), 1);
    sem_post(&image_semaphore);
}

void VideoRenderer::update(attitude_msg attitude)
{
    widgets::los::update(attitude.pitch, attitude.roll, attitude.yaw);
}

void VideoRenderer::update(voltage_msg voltage)
{
    widgets::systemstatus::updateMotorVoltageIn(voltage.motor_voltage);
    widgets::throttlestate::updateVoltageIn(voltage.motor_voltage);
}

void VideoRenderer::update(actuators_state_msg actuators)
{
    widgets::systemstatus::updateRemoteSystemState(actuators.system_state);
    widgets::systemstatus::updateMotorVoltageOut(actuators.throttle_state);
    widgets::throttlestate::updateThrottle(actuators.throttle_state);
}

void VideoRenderer::update(target_msg targets)
{
    widgets::targets::update(targets);
}


void VideoRenderer::toggle_widget(bool enabled, int widget)
{
    widgets::is_enabled[widget] = enabled;
}


void VideoRenderer::toggle_videorec(int fps)
{
    if (!save_frame)
    {
        start_videorec(fps);

        save_frame = true;
    }
    else
    {
        video->release();
        delete video;

        save_frame = false;
    }

    widgets::is_enabled[widgets::VIDEO_REC] = save_frame;
}


void VideoRenderer::run()
{
    while (!stopped)
    {
        render_window();
    }

    if (save_frame)
    {
        video->release();
        delete video;

        save_frame = false;
    }
}


void VideoRenderer::render_window()
{
    sem_wait(&image_semaphore);
    cv::Size size = imagewindow->size();
    widgets::los::draw(imagewindow, 10U + widgets::los::LOS_RAY, size.height - widgets::los::LOS_RAY -  10U);
    widgets::throttlestate::draw(imagewindow, size.width - 320U, size.height - 30U);
    widgets::targets::draw(imagewindow);
    widgets::help::draw(imagewindow, size.width/2, size.height/2);
    widgets::js_help::draw(imagewindow, size.width/2, size.height/2);
    widgets::videorec::draw(imagewindow, size.width - 40U, 50U);
    widgets::systemstatus::draw(imagewindow, size.width - 130U, size.height/2);
    //widgets::devmode::draw(imagewindow, size.width/2, size.height/2);

    cv::imshow(PROJNAME, *imagewindow);
    if (save_frame)
    {
        video->write(*imagewindow);
    }

    sem_post(&image_semaphore);
    usleep(render_timeout_micros);
}

void VideoRenderer::start_videorec(int fps)
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
    save_frame = true;
}
