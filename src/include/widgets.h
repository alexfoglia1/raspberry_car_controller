#ifndef WIDGETS_H
#define WIDGETS_H

#include "defs.h"

#include <stdint.h>
#include <map>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

namespace widgets
{
    extern const int THROTTLESTATE;
    extern const int LOS;
    extern const int COMMANDS_OUT;
    extern const int TARGETS;
    extern const int MENU;
    extern const int VIDEO_REC;
    extern const int SYSTEM_STATUS;
    extern const int JS_MENU;
    extern const int DEVELOPER_MODE;

    extern const int DEV_ATTITUDE_TAB;
    extern const int DEV_VOLTAGE_TAB;
    extern const int DEV_SPEEDTEST_TAB;
    extern const int DEV_EDITPARAMS_TAB;

    extern bool* is_enabled;

    namespace commands_out
    {
        extern char* cmd_display;

        void init();
        void update(int key);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace throttlestate
    {
        extern char* throttle_display;
        extern char* speed_display_kmh;
        extern char* speed_display_ms;
        extern float duty_cycle;

        void init();
        void update(throttle_msg rx);
        void updateDutyCycle(float dc);
        void updateVoltageIn(float voltage_in);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace los
    {
        extern int H_FOV_DEG;
        extern int LOS_RAY;

        extern float act_yaw_deg;
        extern float act_pitch_deg;
        extern float act_roll_deg;

        void init();
        void update(attitude_msg msg);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace targets
    {
        extern target_data* active_targets;
        extern int active_size;

        void init();
        void update(target_msg msg);
        void draw(cv::Mat* imagewindow);
    }

    namespace menu
    {
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace js_menu
    {
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace videorec
    {
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace systemstatus
    {
        extern bool tegra_status;
        extern bool att_status;
        extern bool vid_status;
        extern bool js_status;
        extern bool arduino_status;
        extern float motor_voltage_in;
        extern float motor_voltage_out;
        extern float duty_cycle;

        void init();
        void updateCbit(cbit_result_msg msg);
        void updateMotorVoltageIn(voltage_msg msg);
        void updateMotorVoltageOut(throttle_msg msg);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace devmode
    {
        extern int act_tab;
        extern float* pitch_values;
        extern float* roll_values;
        extern float* yaw_values;
        extern float* vin_values;
        extern float duty_cycle;
        extern int fps;
        extern int rx_timeout_s;
        extern int key_timeout_millis;
        extern int paramToEdit;
        extern double test_t0;
        extern double test_tf;
        extern std::list<float> voltagesIn;
        extern std::list<float> voltagesOut;

        void init();
        void updateVoltageIn(float v);
        void updateVoltageOut(float dc);
        void updateRoll(float r);
        void updatePitch(float p);
        void updateYaw(float y);
        void signalStartSpeedTest();
        void signalStopSpeedTest();
        void signalResetTest();
        void updateSpeedTestVoltageIn(float v);
        void updateTab(int tab);
        void signalSwitchCurrentParam(int delta);
        void editCurrentParamValue(int delta);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

}


#endif // WIDGETS_H
