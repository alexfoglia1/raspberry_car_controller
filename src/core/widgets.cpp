#include "widgets.h"
#include "keymap.h"
#include "utils.h"

#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const int widgets::THROTTLESTATE = 0;
const int widgets::LOS = 1;
const int widgets::COMMANDS_OUT = 2;
const int widgets::TARGETS = 3;
const int widgets::MENU = 4;
const int widgets::VIDEO_REC = 5;
const int widgets::SYSTEM_STATUS = 6;
const int widgets::JS_MENU = 7;
const int widgets::DEVELOPER_MODE = 8;
const int widgets::DEV_ATTITUDE_TAB = 0;
const int widgets::DEV_VOLTAGE_TAB = 1;
const int widgets::DEV_SPEEDTEST_TAB = 2;
const int widgets::DEV_EDITPARAMS_TAB = 3;

const cv::Scalar blue(255, 0, 0);
const cv::Scalar green(0, 255, 0);
const cv::Scalar red(0, 0, 255);
const cv::Scalar yellow(0, 255, 255);
const cv::Scalar gray(200, 200, 200);
const cv::Scalar black(0, 0, 0);

const cv::Scalar bgCol = blue;
const cv::Scalar fgCol = gray;
const cv::Scalar okBatteryCol = green;
const cv::Scalar warnBatteryCol = yellow;
const cv::Scalar failBatteryCol = red;
const cv::Scalar voltageInCol = yellow;
const cv::Scalar voltageOutCol = green;
const cv::Scalar rollCol = red;
const cv::Scalar pitchCol = green;
const cv::Scalar yawCol = yellow;
                                        /*spd   los   cmd   tgt   help    vrec   stat   js    DEV_MODE*/
bool* widgets::is_enabled = new bool[9] {true, true, true, true, false, false, true, false, false};

/** COMMANDS OUT WIDGET **/
char* widgets::commands_out::cmd_display = new char[256];
void widgets::commands_out::init()
{
    sprintf(cmd_display, "OUT: NONE");
}

void widgets::commands_out::update(int key)
{
    sprintf(cmd_display, "OUT: %s", getNameOfKey(key));
}

void widgets::commands_out::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[COMMANDS_OUT])
    {
        std::string text_command(cmd_display);
        int recwidth = 110U;
        int recheight = 20U;
        filledRoundedRectangle(*imagewindow, cv::Point(x, y), cv::Size(recwidth, recheight), bgCol, cv::LINE_AA, 1, 0.1f);
        cv::putText(*imagewindow, text_command, cv::Point2d(x + 5U, y + 15U), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
    }
}

/** THROTTLESTATE WIDGET **/
char* widgets::throttlestate::throttle_display = new char[256];
char* widgets::throttlestate::speed_display_kmh = new char[256];
char* widgets::throttlestate::speed_display_ms = new char[256];
float widgets::throttlestate::duty_cycle = 0.f;
void widgets::throttlestate::init()
{
    sprintf(throttle_display, "[                                                                                                    ]");
    sprintf(speed_display_kmh, "0.00 km/h");
    sprintf(speed_display_ms, "0.00 m/s");
}

void widgets::throttlestate::update(throttle_msg rx)
{
    char th_progress[100];
    float perc = ((float)rx.throttle_state / 255.0) * 100.0;

    for(uint8_t i = 0; i < 100; i++)
    {
        th_progress[i] = (i < perc) ? '*' : ' ';
    }

    sprintf(throttle_display, "[%s]", th_progress);
    duty_cycle = rx.throttle_state /255.f;
}

void widgets::throttlestate::updateVoltageIn(float voltage_in)
{
    sprintf(speed_display_kmh, "%.2f km/h", 3.6f * linearSpeed(voltage_in * duty_cycle));
    sprintf(speed_display_ms, "%.2f m/s", linearSpeed(voltage_in * duty_cycle));
}

void widgets::throttlestate::draw(cv::Mat* imagewindow, int x, int y)
{

    if (is_enabled[THROTTLESTATE])
    {
        std::string text_throttle(throttle_display);
        int recwidth = 3 * strlen(throttle_display);
        int recheight = 20U;
        filledRoundedRectangle(*imagewindow, cv::Point(x, y), cv::Size(recwidth, recheight), bgCol, cv::LINE_AA, 1, 0.1f);
        cv::putText(*imagewindow, text_throttle, cv::Point2d(x + 10U, y + 10U), cv::FONT_HERSHEY_SIMPLEX, 0.175, fgCol, 1, cv::LINE_AA);

        filledRoundedRectangle(*imagewindow, cv::Point(x + recwidth/2 - recwidth/10, y-recheight*2), cv::Size(recwidth/5, recheight * 2 - 5U), bgCol, cv::LINE_AA, 1, 0.1f);
        cv::putText(*imagewindow, speed_display_kmh, cv::Point2d(x + recwidth/2 - recwidth/10, y - recheight - 10U), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, speed_display_ms, cv::Point2d(x + recwidth/2 - recwidth/10, y - recheight + 10U), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
    }
}

/** LOS WIDGET **/
int widgets::los::H_FOV_DEG = 53;
int widgets::los::LOS_RAY = 125;

float widgets::los::act_yaw_deg;
float widgets::los::act_pitch_deg;
float widgets::los::act_roll_deg;

void widgets::los::init()
{
    act_yaw_deg = 0.0;
    act_pitch_deg = 0.0;
    act_roll_deg = 0.0;
}

void widgets::los::update(attitude_msg attitude)
{
    act_yaw_deg = -normalizeAngle((attitude.yaw));
    act_pitch_deg = normalizeAngle((attitude.pitch));
    act_roll_deg = normalizeAngle((attitude.roll));
}

void widgets::los::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[LOS])
    {
        int recwidth = 2 * LOS_RAY;
        int recheight = LOS_RAY;
        int r_brown = 115;
        int g_brown = 51;
        int b_brown = 8;

        filledRoundedRectangle(*imagewindow, cv::Point(x - LOS_RAY, y - LOS_RAY), cv::Size(recwidth, recheight), bgCol, cv::LINE_AA, 1, 0.001f);
        filledRoundedRectangle(*imagewindow, cv::Point(x - LOS_RAY, y), cv::Size(recwidth, recheight), cv::Scalar(b_brown, g_brown, r_brown), cv::LINE_AA, 1, 0.001f);

        double left_bound_deg = normalizeAngle(toRadians((act_yaw_deg - H_FOV_DEG / 2)));
        double right_bound_deg = normalizeAngle(toRadians((act_yaw_deg + H_FOV_DEG / 2)));
        double pitch90 = -asin(sin((atan2(sin(act_pitch_deg * 3.14/180.0), cos(act_pitch_deg * 3.14/180.0)) * 180.0/3.14) * 3.14/180.0)) * 180.0/3.14;
        double elevPercentage = pitch90 / 90;

        cv::Point2d losCenter(x, y);
        cv::Point2d losLeft(losCenter.x + LOS_RAY * cos(toRadians(left_bound_deg - 90)), losCenter.y + LOS_RAY * sin(toRadians(left_bound_deg - 90)));
        cv::Point2d losRight(losCenter.x + LOS_RAY * cos(toRadians(right_bound_deg - 90)), losCenter.y + LOS_RAY * sin(toRadians(right_bound_deg - 90)));
        cv::Point2d losElev(losCenter.x, losCenter.y + elevPercentage * LOS_RAY);
        cv::Point2d losRollRight(losCenter.x + (2 * LOS_RAY / 3) * cos(toRadians(act_roll_deg)), losCenter.y + (2 * LOS_RAY / 3) * sin(toRadians(act_roll_deg)));
        cv::Point2d losRollLeft(losCenter.x - (2 * LOS_RAY / 3) * cos(toRadians(act_roll_deg)), losCenter.y - (2 * LOS_RAY / 3) * sin(toRadians(act_roll_deg)));

        /** Plot actual attitude **/
        cv::line(*imagewindow, losCenter, losLeft, green, 1);
        cv::line(*imagewindow, losCenter, losRight, green, 1);
        cv::line(*imagewindow, losRollLeft, losRollRight, green, 1);
        cv::drawMarker(*imagewindow, losElev, green, cv::MARKER_CROSS, 20, 1, cv::LINE_AA);

        /** Scales **/
        /** External ellipse: for yaw **/
        cv::ellipse(*imagewindow, losCenter, cv::Size(LOS_RAY, LOS_RAY), 15, 0, 360, fgCol, 1);
        double grad_scale_yaw_deg = 0.0;
        while (grad_scale_yaw_deg < 360)
        {
            char buf[64];
            sprintf(buf, "%.1f", grad_scale_yaw_deg);
            double notch_start_x = losCenter.x + LOS_RAY * cos(toRadians(grad_scale_yaw_deg - 90));
            double notch_start_y = losCenter.y + LOS_RAY * sin(toRadians(grad_scale_yaw_deg - 90));
            double notch_end_x = losCenter.x + (LOS_RAY - 10) * cos(toRadians(grad_scale_yaw_deg - 90));
            double notch_end_y = losCenter.y + (LOS_RAY - 10) * sin(toRadians(grad_scale_yaw_deg - 90));
            double txt_x = losCenter.x + (LOS_RAY - 20) * cos(toRadians(grad_scale_yaw_deg - 90)) - 10;
            double txt_y = losCenter.y + (LOS_RAY - 20) * sin(toRadians(grad_scale_yaw_deg - 90));
            cv::Point2d notch_start(notch_start_x, notch_start_y);
            cv::Point2d notch_end(notch_end_x, notch_end_y);

            cv::line(*imagewindow, notch_start, notch_end, fgCol, 1);
            cv::putText(*imagewindow, cv::String(buf), cv::Point2d(txt_x, txt_y), cv::FONT_HERSHEY_SIMPLEX, 0.25, fgCol, 1, cv::LINE_AA);

            grad_scale_yaw_deg += 45.0/2;
        }
        /** Internal ellipse: for roll **/
        cv::ellipse(*imagewindow, losCenter, cv::Size(2 * LOS_RAY / 3, 2 * LOS_RAY / 3), 15, 0, 360, fgCol, 1);
        double grad_scale_roll_deg = 0.0;
        while (grad_scale_roll_deg < 360)
        {
            char buf[64];
            if (grad_scale_roll_deg < 180)
            {
                sprintf(buf, "%.1f", grad_scale_roll_deg);
            }
            else
            {
                sprintf(buf, "%.1f", grad_scale_roll_deg - 180);
            }
            double notch_start_x = losCenter.x + (2 * LOS_RAY / 3) * cos(toRadians(grad_scale_roll_deg));
            double notch_start_y = losCenter.y + (2 * LOS_RAY / 3) * sin(toRadians(grad_scale_roll_deg));
            double notch_end_x = losCenter.x + (2 * LOS_RAY / 3 - 10) * cos(toRadians(grad_scale_roll_deg));
            double notch_end_y = losCenter.y + (2 * LOS_RAY / 3 - 10) * sin(toRadians(grad_scale_roll_deg));
            double txt_x = losCenter.x + (2 * LOS_RAY / 3 - 20) * cos(toRadians(grad_scale_roll_deg)) - 10;
            double txt_y = losCenter.y + (2 * LOS_RAY / 3 - 20) * sin(toRadians(grad_scale_roll_deg));
            cv::Point2d notch_start(notch_start_x, notch_start_y);
            cv::Point2d notch_end(notch_end_x, notch_end_y);

            cv::line(*imagewindow, notch_start, notch_end, fgCol, 1);
            cv::putText(*imagewindow, cv::String(buf), cv::Point2d(txt_x, txt_y), cv::FONT_HERSHEY_SIMPLEX, 0.25, fgCol, 1, cv::LINE_AA);

            grad_scale_roll_deg += 45.0/2;
        }

        double grad_scale_pitch_deg = -90;
        while (grad_scale_pitch_deg < 90)
        {

            double elevPercentage = grad_scale_pitch_deg / 90;
            double notch_start_x = losCenter.x - 5;
            double notch_start_y = losCenter.y + elevPercentage * LOS_RAY;
            double notch_end_x = losCenter.x + 5;
            double notch_end_y = losCenter.y + elevPercentage * LOS_RAY;

            cv::Point2d notch_start(notch_start_x, notch_start_y);
            cv::Point2d notch_end(notch_end_x, notch_end_y);
            cv::line(*imagewindow, notch_start, notch_end, fgCol, 1);

            grad_scale_pitch_deg += 45.0/2;
        }
    }
}

/** TARGETS WIDGET **/
target_data* widgets::targets::active_targets = new target_data[MAX_TARGETS];
int widgets::targets::active_size;

void widgets::targets::init()
{
    active_size = 0;
}

void widgets::targets::update(target_msg target)
{
    uint8_t n_targets = std::min<uint8_t>(MAX_TARGETS, target.n_targets);
    widgets::targets::active_size = n_targets;
    for(uint8_t i = 0; i < n_targets; i++)
    {
        active_targets[i] = target.data[i];
    }
}

void widgets::targets::draw(cv::Mat* imagewindow)
{
    if (is_enabled[TARGETS])
    {
        for ( int i = 0; i < active_size; i++ )
        {
           target_data target = active_targets[i];
           int x0 = target.x_pos;
           int y0 = target.y_pos;

           cv::Rect target_rect(x0, y0, target.width, target.height);
           cv::rectangle(*imagewindow, target_rect, cv::Scalar(255,255,0));

           char name_and_prob[150];
           sprintf(name_and_prob, "%s %f", target.description, target.confidence);

           cv::putText(*imagewindow, cv::String(name_and_prob), cv::Point(x0 + 4.0*target_rect.width/5.0, y0 + 10), cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(255,0,0), 1, cv::LINE_AA);
        }

    }
}

/** MENU WIDGET **/
void widgets::menu::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[MENU])
    {
        int lineSpacing = 20;
        int offsetX = 30;
        int width = 300;
        int height = 21 * (lineSpacing + 2);
        int centerx = x;
        int centery = y;
        int x0 = centerx - width/2;
        int y0 = centery - height/2;

        filledRoundedRectangle(*imagewindow, cv::Point(x0, y0), cv::Size(width, height), bgCol, cv::LINE_AA, 1, 0.1f);

        cv::putText(*imagewindow, cv::String("COMMANDS OUT"), cv::Point(x0 + offsetX, y0 + lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("SET MOVE FORWARD [w]"), cv::Point(x0 + 2 * offsetX, y0 + 2 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("SET MOVE BACKWARD [x]"), cv::Point(x0 + 2 * offsetX, y0 + 3 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("SET TURN LEFT [a]"), cv::Point(x0 + 2 * offsetX, y0 + 4 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("SET TURN RIGHT [d]"), cv::Point(x0 + 2 * offsetX, y0 + 5 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("SET THROTTLE MAX [<space>]"), cv::Point(x0 +  2 * offsetX, y0 + 6 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("SET THROTTLE MIN [<backspace>]"), cv::Point(x0 + 2 * offsetX, y0 + 7 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("THROTTLE UP 1 [n]"), cv::Point(x0 + 2 * offsetX, y0 + 8 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("THROTTLE DOWN 1 [b]"), cv::Point(x0 + 2 * offsetX, y0 + 9 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("THROTTLE UP 10 [p]"), cv::Point(x0 + 2 * offsetX, y0 + 10 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("THROTTLE DOWN 10 [q]"), cv::Point(x0 + 2 * offsetX, y0 + 11 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);

        cv::putText(*imagewindow, cv::String("LOCAL COMMANDS"), cv::Point(x0 + offsetX, y0 + 12 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE THROTTLE WIDGET [s]"), cv::Point(x0 + 2 * offsetX, y0 + 13 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE LOS WIDGET [l]"), cv::Point(x0 + 2 * offsetX, y0 + 14 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE COMMAND OUT WIDGET [c]"), cv::Point(x0 + 2 * offsetX, y0 + 15 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE TARGET WIDGET [m]"), cv::Point(x0 + 2 * offsetX, y0 + 16 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE HELP [h]"), cv::Point(x0 + 2 * offsetX, y0 + 17 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE JOYSTICK HELP [j / <select>]"), cv::Point(x0 + 2 * offsetX, y0 + 18 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE SYSTEM STATUS [y]"), cv::Point(x0 + 2 * offsetX, y0 + 19 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("VIDEO RECORD [v]"), cv::Point(x0 + 2 * offsetX, y0 + 20 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("ENTER DEVELOPMENT MODE [e]"), cv::Point(x0 + 2 * offsetX, y0 + 21 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);

    }
}

/** VIDEOREC WIDGET **/
void widgets::videorec::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[VIDEO_REC])
    {
        filledRoundedRectangle(*imagewindow, cv::Point(x, y), cv::Size(15, 15), cv::Scalar(0,0,255), cv::LINE_AA, 1, 0.5f);
    }
}

/** SYSTEMSTATUS WIDGET **/
bool widgets::systemstatus::tegra_status;
bool widgets::systemstatus::att_status;
bool widgets::systemstatus::vid_status;
bool widgets::systemstatus::js_status;
bool widgets::systemstatus::arduino_status;
float widgets::systemstatus::motor_voltage_in;
float widgets::systemstatus::motor_voltage_out;
float widgets::systemstatus::duty_cycle;

void widgets::systemstatus::init()
{
    tegra_status = false;
    att_status = false;
    vid_status = false;
    js_status = false;
    motor_voltage_in = 0.f;
    motor_voltage_out = 0.f;
    duty_cycle = 0.f;
}

void widgets::systemstatus::updateCbit(cbit_result_msg msg)
{
    tegra_status = msg.tegra_failure == false;
    att_status = msg.att_failure == false;
    vid_status = msg.vid_failure == false;
    js_status  = msg.js_failure  == false;
    arduino_status = msg.arduino_failure == false;
}

void widgets::systemstatus::updateMotorVoltageIn(voltage_msg msg)
{
    motor_voltage_in = msg.motor_voltage;
    motor_voltage_out = motor_voltage_in * duty_cycle;
}

void widgets::systemstatus::updateMotorVoltageOut(throttle_msg msg)
{
    duty_cycle = (float)msg.throttle_state / (float)std::numeric_limits<uint8_t>::max();
    motor_voltage_out = motor_voltage_in * duty_cycle;
}

void widgets::systemstatus::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[SYSTEM_STATUS])
    {
        int lineSpacing = 20;
        int offsetX = 5;
        int width = 210;
        int height = 8 * (lineSpacing + 2);
        int centerx = x;
        int centery = y;
        int x0 = centerx - width/2;
        int y0 = centery - height/2;

        float battery_charge_percentage = motor_voltage_in / 10.f; //MAX VOLTAGE 10.0 V
        char prompt_motor_voltage_in[64];
        char prompt_motor_voltage_out[64];

        sprintf(prompt_motor_voltage_in, "%.1f V", motor_voltage_in);
        sprintf(prompt_motor_voltage_out, "%.1f V", motor_voltage_out);

        filledRoundedRectangle(*imagewindow, cv::Point(x0, y0), cv::Size(width, height), bgCol, cv::LINE_AA, 1, 0.1f);
        cv::putText(*imagewindow, cv::String("TEGRA DETECTOR"), cv::Point(x0 + offsetX, y0 + 1 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("LSM9DS1 IMU"), cv::Point(x0 + offsetX, y0 + 2 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("VIDEO CAMERA"), cv::Point(x0 + offsetX, y0 + 3 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("ARDUINO NANO"), cv::Point(x0 +  offsetX, y0 + 4 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("MOTOR BATTERY STATUS"), cv::Point(x0 +  offsetX, y0 + 6 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("MOTOR VOLTAGE IN"), cv::Point(x0 +  offsetX, y0 + 7 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("MOTOR VOLTAGE OUT"), cv::Point(x0 +  offsetX, y0 + 8 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("JOYSTICK"), cv::Point(x0 +  offsetX, y0 + 5 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);

        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 1 * lineSpacing - 9), cv::Size(15, 15), tegra_status ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 2 * lineSpacing - 9), cv::Size(15, 15), att_status ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 3 * lineSpacing - 9), cv::Size(15, 15), vid_status ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 4 * lineSpacing - 9), cv::Size(15, 15), arduino_status  ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 6 * lineSpacing - 9), cv::Size(15, 15),
                               battery_charge_percentage >= 0.70 ? okBatteryCol :
                               battery_charge_percentage >= 0.40 ? warnBatteryCol : failBatteryCol, cv::LINE_AA, 1, 0.5f);
        cv::putText(*imagewindow, cv::String(prompt_motor_voltage_in), cv::Point(x0 + 30 * offsetX, y0 + 7 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String(prompt_motor_voltage_out), cv::Point(x0 + 30 * offsetX, y0 + 8 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 5 * lineSpacing - 9), cv::Size(15, 15), js_status  ? green : red, cv::LINE_AA, 1, 0.5f);
    }
}

/** JS MENU WIDGET **/
void widgets::js_menu::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[JS_MENU])
    {
        int lineSpacing = 20;
        int offsetX = 30;
        int width = 300;
        int height = 12 * (lineSpacing + 2);
        int centerx = x;
        int centery = y;
        int x0 = centerx - width/2;
        int y0 = centery - height/2;
        filledRoundedRectangle(*imagewindow, cv::Point(x0, y0), cv::Size(width, height), bgCol, cv::LINE_AA, 1, 0.1f);
        cv::putText(*imagewindow, cv::String("COMMANDS OUT"), cv::Point(x0 + offsetX, y0 + lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("ACCELERATE [R2]"), cv::Point(x0 + 2 * offsetX, y0 + 2 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("BREAK [L2]"), cv::Point(x0 + 2 * offsetX, y0 + 3 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TURN [L3]"), cv::Point(x0 + 2 * offsetX, y0 + 4 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("LOCAL COMMANDS"), cv::Point(x0 + offsetX, y0 + 5 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("VIDEO RECORD [start]"), cv::Point(x0 + 2 * offsetX, y0 + 6 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE JOYSTICK HELP [select]"), cv::Point(x0 + 2 * offsetX, y0 + 7 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE THROTTLE WIDGET [cross]"), cv::Point(x0 + 2 * offsetX, y0 + 8 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE SYSTEM STATUS [circle]"), cv::Point(x0 + 2 * offsetX, y0 + 9 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE COMMAND OUT WIDGET [triangle]"), cv::Point(x0 + 2 * offsetX, y0 + 10 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE LOS WIDGET [square]"), cv::Point(x0 + 2 * offsetX, y0 + 11 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("ENTER DEVELOPER MODE [R1]"), cv::Point(x0 + 2 * offsetX, y0 + 12 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
    }
}

/** DEV MODE **/
int widgets::devmode::act_tab;
float* widgets::devmode::vin_values;
float* widgets::devmode::pitch_values;
float* widgets::devmode::roll_values;
float* widgets::devmode::yaw_values;
float widgets::devmode::duty_cycle;
int widgets::devmode::fps;
int widgets::devmode::rx_timeout_s;
int widgets::devmode::key_timeout_millis;
int widgets::devmode::paramToEdit;
double widgets::devmode::test_t0;
double widgets::devmode::test_tf;
std::list<float> widgets::devmode::voltagesIn;
std::list<float> widgets::devmode::voltagesOut;

void widgets::devmode::init()
{
    act_tab = DEV_ATTITUDE_TAB;

    fps = 100;
    rx_timeout_s = 5;
    key_timeout_millis = 10;
    paramToEdit = 0; /*FPS*/

    vin_values = new float[100];
    memset(vin_values, 0x00, 100 * sizeof(float));
    pitch_values = new float[100];
    memset(pitch_values, 0x00, 100 * sizeof(float));
    roll_values = new float[100];
    memset(roll_values, 0x00, 100 * sizeof(float));
    yaw_values = new float[100];
    memset(yaw_values, 0x00, 100 * sizeof(float));

    test_t0 = -1.0;
    test_tf = -1.0;
}

void widgets::devmode::updatePitch(float p)
{
    static int llpitch = -1;

    if (llpitch < 99)
    {
        llpitch += 1;
    }
    else
    {
        llpitch = 0;
    }
    pitch_values[llpitch] = normalizeAngle(p);
}

void widgets::devmode::updateRoll(float r)
{
    static int llroll = -1;

    if (llroll < 99)
    {
        llroll += 1;
    }
    else
    {
        llroll = 0;
    }
    roll_values[llroll] = normalizeAngle(r);
}

void widgets::devmode::updateYaw(float y)
{
    static int llyaw = -1;

    if (llyaw < 99)
    {
        llyaw += 1;
    }
    else
    {
        llyaw = 0;
    }
    yaw_values[llyaw] = normalizeAngle(y);
}

void widgets::devmode::updateVoltageIn(float v)
{
    static int llvin = -1;

    if (llvin < 99)
    {
        llvin += 1;
    }
    else
    {
        llvin = 0;
    }
    vin_values[llvin] = v;
}

void widgets::devmode::updateVoltageOut(float dc)
{
    duty_cycle = dc;
}

void widgets::devmode::updateTab(int tab)
{
    act_tab = tab;
    if (act_tab != DEV_SPEEDTEST_TAB)
    {
        signalResetTest();
    }
}

void widgets::devmode::signalStartSpeedTest()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    test_t0 = (double)tv.tv_sec + ((double)(tv.tv_usec) * 1e-6);
    test_tf = -1;
}

void widgets::devmode::signalStopSpeedTest()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    test_tf = (double)tv.tv_sec + ((double)(tv.tv_usec) * 1e-6);
}

void widgets::devmode::signalResetTest()
{
    test_t0 = -1;
    test_tf = -1;
    voltagesIn.clear();
    voltagesOut.clear();
}

void widgets::devmode::updateSpeedTestVoltageIn(float v)
{
    voltagesIn.push_back(v);
    voltagesOut.push_back(v * duty_cycle);
}

void widgets::devmode::signalSwitchCurrentParam(int delta)
{
    paramToEdit += delta;
    if (paramToEdit > 2) paramToEdit = 0;
    if (paramToEdit < 0) paramToEdit = 2;
}

void widgets::devmode::editCurrentParamValue(int delta)
{
    switch (paramToEdit)
    {
    case 0: /*FPS*/
        if (fps + delta < 1) fps = 1; else fps += delta;
        break;
    case 1: /*RX TIMEOUT*/
        if (rx_timeout_s + delta < 1) rx_timeout_s = 1; else rx_timeout_s += delta;
        break;
    case 2: /*WAIT KEY TIMEOUT*/
        if (key_timeout_millis + delta < 1) key_timeout_millis = 1; else key_timeout_millis += delta;
        break;
    }
}

void widgets::devmode::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[DEVELOPER_MODE])
    {
        /** Local variable initialization **/
        int lineSpacing = 20;
        int width = 900;
        int height = 600;
        int centerx = x;
        int centery = y;
        int x0 = centerx - width/2;
        int y0 = centery - height/2;
        int selection_x = act_tab == DEV_ATTITUDE_TAB ? 175 :
                          act_tab == DEV_VOLTAGE_TAB ? 275 :
                          act_tab == DEV_SPEEDTEST_TAB ? 375 : 475;
        int selection_y = y0 + lineSpacing / 2 - 28;
        int selection_width = act_tab == DEV_ATTITUDE_TAB ? 93:
                              act_tab == DEV_VOLTAGE_TAB ?  90 :
                              act_tab == DEV_SPEEDTEST_TAB ? 77 : 120;
        int selection_height = 3 * lineSpacing / 5 + 3;
        float scale_x = width / 100.f;
        const cv::Scalar* fgColAttitudeTab = act_tab == DEV_ATTITUDE_TAB ? &bgCol : &fgCol;
        const cv::Scalar* fgColVoltageTab = act_tab == DEV_VOLTAGE_TAB ? &bgCol : &fgCol;
        const cv::Scalar* fgColSpeedTestTab = act_tab == DEV_SPEEDTEST_TAB ? &bgCol : &fgCol;
        const cv::Scalar* fgColEditParamsTab = act_tab == DEV_EDITPARAMS_TAB ? &bgCol : &fgCol;
        static double delta_t_test = 0.f;
        static double avg_vin_test = 0.f;
        static double avg_vout_test = 0.f;

        /** Developer mode background **/
        filledRoundedRectangle(*imagewindow, cv::Point(x0, y0 - 20), cv::Size(width, height + 20), bgCol, cv::LINE_AA, 1, 0.01f);
        cv::rectangle(*imagewindow, cv::Rect(x0, y0 - 2, width, height + 2), fgCol);
        float scale_y = 1.f;
        float delta_value_y = 1.f;
        float max_value_y = 1.f;
        bool isPlot = false;

        /** Developer mode header **/
        filledRoundedRectangle(*imagewindow, cv::Point(selection_x, selection_y), cv::Size(selection_width, selection_height), fgCol, cv::LINE_AA, 1, 0.01f);
        cv::putText(*imagewindow, cv::String("ATTITUDE PLOT"), cv::Point(175, y0 + lineSpacing - 25), cv::FONT_HERSHEY_SIMPLEX, 0.4, *fgColAttitudeTab, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("VOLTAGE PLOT"), cv::Point(275, y0 + lineSpacing - 25), cv::FONT_HERSHEY_SIMPLEX, 0.4, *fgColVoltageTab, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("SPEED TEST"), cv::Point(375, y0 + lineSpacing - 25), cv::FONT_HERSHEY_SIMPLEX, 0.4, *fgColSpeedTestTab, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("EDIT PARAMETERS"), cv::Point(475, y0 + lineSpacing - 25), cv::FONT_HERSHEY_SIMPLEX, 0.4, *fgColEditParamsTab, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("NEXT TAB [g/<R3>] PREV. TAB [f/<L3>]"), cv::Point(width - x0, y0 + lineSpacing - 25), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);

        /** Developer mode plot axis **/
        switch(act_tab)
        {
        case DEV_ATTITUDE_TAB:
            isPlot = true;
            delta_value_y = 45.f/2.f;
            max_value_y = 360.f;
            scale_y = height / max_value_y;
            break;
        case DEV_VOLTAGE_TAB:
            delta_value_y = 0.25f;
            isPlot = true;
            max_value_y = 10.f;
            scale_y = height / max_value_y;
            break;
        default:
            break;
        }
        if (isPlot)
        {
            float value_y = 0.f;

            while(value_y < max_value_y)
            {
                float height_of_value = (height + y0) - value_y * scale_y;
                char buf[64];
                sprintf(buf, "%.2f", value_y);
                cv::putText(*imagewindow, cv::String(buf), cv::Point(x0, height_of_value - 2), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);

                float act_x = x0;
                bool drawSegment = true;
                float segLen = width / 300.f;
                while(act_x < x0 + width)
                {
                    if(drawSegment && fabs(value_y) >= delta_value_y)
                    {
                        cv::line(*imagewindow, cv::Point(act_x, height_of_value), cv::Point(act_x + segLen, height_of_value), fgCol,1, cv::LINE_AA);
                    }

                    act_x += segLen;
                    drawSegment = !drawSegment;
                }
                value_y += delta_value_y;
            }
        }

        /** Developer mode content **/
        switch(act_tab)
        {
        case DEV_ATTITUDE_TAB:
            cv::putText(*imagewindow, cv::String("YAW"), cv::Point(x0 + width - 200, y0 + 13), cv::FONT_HERSHEY_SIMPLEX, 0.35, yawCol, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("PITCH"), cv::Point(x0 + width - 150, y0 + 13), cv::FONT_HERSHEY_SIMPLEX, 0.35, pitchCol, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("ROLL"), cv::Point(x0 + width - 100, y0 + 13), cv::FONT_HERSHEY_SIMPLEX, 0.35, rollCol, 1, cv::LINE_AA);
            for(int i = 0; i < 99; i++)
            {
                float x1 = x0 + i * scale_x;
                float y1 = (height + y0) - pitch_values[i] * scale_y;
                float x2 = x0 + (i + 1) * scale_x;
                float y2 = (height + y0) - pitch_values[i + 1] * scale_y;
                cv::line(*imagewindow, cv::Point(x1, y1), cv::Point(x2, y2), pitchCol);

                x1 = x0 + i * scale_x;
                y1 = (height + y0) - roll_values[i] * scale_y;
                x2 = x0 + (i + 1) * scale_x;
                y2 = (height + y0) - roll_values[i + 1] * scale_y;
                cv::line(*imagewindow, cv::Point(x1, y1), cv::Point(x2, y2), rollCol);

                x1 = x0 + i * scale_x;
                y1 = (height + y0) - yaw_values[i] * scale_y;
                x2 = x0 + (i + 1) * scale_x;
                y2 = (height + y0) - yaw_values[i + 1] * scale_y;
                cv::line(*imagewindow, cv::Point(x1, y1), cv::Point(x2, y2), yawCol);
            }
            break;
        case DEV_VOLTAGE_TAB:
        {
            cv::putText(*imagewindow, cv::String("V IN"), cv::Point(x0 + width - 200, y0 + 13), cv::FONT_HERSHEY_SIMPLEX, 0.35, voltageInCol, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("V OUT"), cv::Point(x0 + width - 150, y0 + 13), cv::FONT_HERSHEY_SIMPLEX, 0.35, voltageOutCol, 1, cv::LINE_AA);
            for(int i = 0; i < 99; i++)
            {
                float x1 = x0 + i * scale_x;
                float y1 = (height + y0) - vin_values[i] * scale_y;
                float x2 = x0 + (i + 1) * scale_x;
                float y2 = (height + y0) - vin_values[i + 1] * scale_y;
                cv::line(*imagewindow, cv::Point(x1, y1), cv::Point(x2, y2), voltageInCol);

                y1 = (height + y0) - duty_cycle * vin_values[i] * scale_y;
                y2 = (height + y0) - duty_cycle * vin_values[i + 1] * scale_y;
                cv::line(*imagewindow, cv::Point(x1, y1), cv::Point(x2, y2), voltageOutCol);
            }
            break;
        }
        case DEV_SPEEDTEST_TAB:
        {
            if (test_t0 > 0 && test_tf < 0)
            {
                /** Test running **/
                struct timeval act_t;
                gettimeofday(&act_t, NULL);
                delta_t_test = (double)act_t.tv_sec + ((double)act_t.tv_usec * 1e-6) - test_t0;
                avg_vin_test = 0.f;
                avg_vout_test = 0.f;

                cv::putText(*imagewindow, cv::String("STOP TEST"), cv::Point(x0 + 20, y0 + 100), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
                cv::putText(*imagewindow, cv::String("[s/<cross>]"), cv::Point(x0 + 300, y0 + 100), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
            }
            else if(test_t0 > 0 && test_tf > 0)
            {
                /** Test finished **/
                delta_t_test = test_tf - test_t0;
                avg_vin_test = avg(voltagesIn);
                avg_vout_test = avg(voltagesOut);

                FILE* f = fopen("speedtest.csv", "a");
                fprintf(f, "%f, %f, %f\n", delta_t_test, avg_vin_test, avg_vout_test);
                fclose(f);

                signalResetTest();
            }
            else
            {
                /** Test is not running **/
                cv::putText(*imagewindow, cv::String("START TEST"), cv::Point(x0 + 20, y0 + 100), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
                cv::putText(*imagewindow, cv::String("[s/<cross>]"), cv::Point(x0 + 300, y0 + 100), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
            }

            char buf[64];

            sprintf(buf, "%f s", (delta_t_test));
            cv::putText(*imagewindow, cv::String("DURATION"), cv::Point(x0 + 20, y), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String(buf), cv::Point(x0 + 300, y), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);

            sprintf(buf, "%f V", avg_vin_test);
            cv::putText(*imagewindow, cv::String("AVG VOLTAGE IN"), cv::Point(x0 + 20, y + 80), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String(buf), cv::Point(x0 + 300, y + 80), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);

            sprintf(buf, "%f V", avg_vout_test);
            cv::putText(*imagewindow, cv::String("AVG VOLTAGE OUT"), cv::Point(x0 + 20, y + 160), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String(buf), cv::Point(x0 + 300, y + 160), cv::FONT_HERSHEY_SIMPLEX, 0.8, fgCol, 2, cv::LINE_AA);
            break;
        }
        case DEV_EDITPARAMS_TAB:
        {
            int selection_x = x0 + 10;
            int selection_y = paramToEdit == 0 ? /*FPS*/ y0 + 10 :
                              paramToEdit == 1 ? /*RX TIMEOUT*/  y0 + 50 : /*WAITKEY TIMEOUT*/  y0 + 90;
            int selection_width = paramToEdit == 0 ? /*FPS*/ 122:
                                  paramToEdit == 1 ? /*RX TIMEOUT*/ 100 : /*WAITKEY TIMEOUT*/ 220;
            const cv::Scalar* fgColFps = paramToEdit == 0 ? &bgCol : &fgCol;
            const cv::Scalar* fgColRxTimeout = paramToEdit == 1 ? &bgCol : &fgCol;
            const cv::Scalar* fgColWaitKeyTimeout = paramToEdit == 2 ? &bgCol : &fgCol;
            filledRoundedRectangle(*imagewindow, cv::Point(selection_x, selection_y), cv::Size(selection_width, selection_height), fgCol, cv::LINE_AA, 1, 0.01f);
            cv::putText(*imagewindow, cv::String("VIDEO RECORD FPS"), cv::Point(x0 + 10, y0 + 10 + selection_height - 2), cv::FONT_HERSHEY_SIMPLEX, 0.4, *fgColFps, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("RX TIMEOUT (s)"), cv::Point(x0 + 10, y0 + 50 + selection_height - 2), cv::FONT_HERSHEY_SIMPLEX, 0.4, *fgColRxTimeout, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("KEYBOARD INPUT TIMEOUT (millis)"), cv::Point(x0 + 10, y0 + 90 + selection_height - 2), cv::FONT_HERSHEY_SIMPLEX, 0.4, *fgColWaitKeyTimeout, 1, cv::LINE_AA);

            char buf[64];
            sprintf(buf, "%d", fps);
            cv::putText(*imagewindow, cv::String(buf), cv::Point(x0 + width / 3, y0 + 10 + selection_height), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
            sprintf(buf, "%d", rx_timeout_s);
            cv::putText(*imagewindow, cv::String(buf), cv::Point(x0 + width / 3, y0 + 50 + selection_height), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
            sprintf(buf, "%d", key_timeout_millis);
            cv::putText(*imagewindow, cv::String(buf), cv::Point(x0 + width / 3, y0 + 90 + selection_height), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);

            cv::putText(*imagewindow, cv::String("SELECTION DOWN [s/cross]"), cv::Point(x0 + 10, y0 + height / 2 + lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("SELECTION UP [c/triangle]"), cv::Point(x0 + 10, y0 + height / 2 + 2 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("VALUE DOWN [l/square]"), cv::Point(x0 + 10, y0 + height / 2 + 3 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
            cv::putText(*imagewindow, cv::String("VALUE UP [y/circle]"), cv::Point(x0 + 10, y0 + height / 2 + 4 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);

            break;
        }
        default:
            break;
        }
    }
}

