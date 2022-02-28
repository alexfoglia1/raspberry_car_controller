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
const int widgets::TARGETS = 2;
const int widgets::HELP = 3;
const int widgets::VIDEO_REC = 4;
const int widgets::SYSTEM_STATUS = 5;
const int widgets::JS_HELP = 6;
const int widgets::DEVELOPER_MODE = 7;

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
                                        /*spd   los  tgt   help    vrec  sysstat      js   DEV_MODE*/
bool* widgets::is_enabled = new bool[8] {true, true, true, false, false, true,     false,  false};


/** Context Menu **/
widgets::ContextMenu::ContextMenu()
{
    visible = false;
    selected_item = 0;
}

void widgets::ContextMenu::show()
{
    visible = true;
}

void widgets::ContextMenu::hide()
{
    visible = false;
}

void widgets::ContextMenu::navigate(int delta)
{
    selected_item += delta;
    if (selected_item >= int(items.size()))
    {
        selected_item = 0;
    }

    if (selected_item < 0)
    {
        selected_item = items.size() - 1;
    }
}

void widgets::ContextMenu::select()
{

}

void widgets::ContextMenu::draw(cv::Mat* frame)
{
    if (visible)
    {
        cv::Size size = frame->size();
        int lineSpacing = 30;
        int recheight = items.size() * (lineSpacing + 2);
        int recwidth = 300;
        filledRoundedRectangle(*frame, cv::Point(size.width / 2 - recwidth/2, size.height/2 - recheight/2), cv::Size(recwidth, recheight), bgCol, cv::LINE_AA, 1, 0.1f);

        cv::Point items_top_left = cv::Point(size.width / 2 - recwidth/2 + lineSpacing/2, size.height/2 - recheight/2 + lineSpacing/2);
        cv::Size selection_size = cv::Size(recwidth/2, lineSpacing/2);
        cv::Point selection_coord = cv::Point(items_top_left.x, items_top_left.y + selected_item * lineSpacing - selection_size.height / 2);
        filledRoundedRectangle(*frame, selection_coord, selection_size, fgCol, cv::LINE_AA, 1, 0.1f);

        qDebug("x(%d), y(%d)\n", selection_coord.x, selection_coord.y);

        ContextMenuItemKey selection = items[selected_item];
        for (int i = 0; i < int(items.size()); i++)
        {
            ContextMenuItemKey actual = items[i];
            cv::Point act_coord(items_top_left.x, items_top_left.y + i * lineSpacing + 3);
            bool actualIsSelected = items[i].item == selection.item;

            cv::putText(*frame, cv::String(items[i].text.toStdString()), act_coord, cv::FONT_HERSHEY_SIMPLEX, 0.35, actualIsSelected ? bgCol : fgCol, 1, cv::LINE_AA);
        }
    }
}


/** THROTTLESTATE WIDGET **/
char* widgets::throttlestate::throttle_display = new char[256];
char* widgets::throttlestate::speed_display_kmh = new char[256];
char* widgets::throttlestate::speed_display_ms = new char[256];
float widgets::throttlestate::duty_cycle = 0.f;
void widgets::throttlestate::init()
{
    char throttle_zero[100];
    memset(throttle_zero, ' ', 99);
    throttle_zero[99] = '\0';

    sprintf(throttle_display, "[%s]", throttle_zero);
    sprintf(speed_display_kmh, "0.00 km/h");
    sprintf(speed_display_ms, "0.00 m/s");
}

void widgets::throttlestate::updateThrottle(uint8_t throttle_state)
{
    char th_progress[100];
    float perc = ((float)throttle_state / 255.0) * 100.0;

    for(uint8_t i = 0; i < 99; i++)
    {
        th_progress[i] = (i < perc) ? '*' : ' ';
    }
    th_progress[99] = '\0';

    sprintf(throttle_display, "[%s]", th_progress);
    duty_cycle = throttle_state /255.f;
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

void widgets::los::update(double pitch, double roll, double yaw)
{
    act_yaw_deg = -normalizeAngle((yaw));
    act_pitch_deg = normalizeAngle((pitch));
    act_roll_deg = normalizeAngle((roll));
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

           cv::putText(*imagewindow, cv::String(name_and_prob), cv::Point(x0 + 4.0*target_rect.width/5.0, y0 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255,0,0), 2, cv::LINE_AA);
        }

    }
}

/** HELP WIDGET **/
void widgets::help::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[HELP])
    {
        int lineSpacing = 20;
        int offsetX = 30;
        int width = 300;
        int height = 11 * (lineSpacing + 2);
        int centerx = x;
        int centery = y;
        int x0 = centerx - width/2;
        int y0 = centery - height/2;

        filledRoundedRectangle(*imagewindow, cv::Point(x0, y0), cv::Size(width, height), bgCol, cv::LINE_AA, 1, 0.1f);

        cv::putText(*imagewindow, cv::String("LOCAL COMMANDS"), cv::Point(x0 + offsetX, y0 +  lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.4, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE THROTTLE WIDGET [s]"), cv::Point(x0 + 2 * offsetX, y0 + 2 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE LOS WIDGET [l]"), cv::Point(x0 + 2 * offsetX, y0 + 3 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE COMMAND OUT WIDGET [c]"), cv::Point(x0 + 2 * offsetX, y0 + 4 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE TARGET WIDGET [m]"), cv::Point(x0 + 2 * offsetX, y0 + 5 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE HELP [h]"), cv::Point(x0 + 2 * offsetX, y0 + 6 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE JOYSTICK HELP [j / <select>]"), cv::Point(x0 + 2 * offsetX, y0 + 7 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("TOGGLE SYSTEM STATUS [y]"), cv::Point(x0 + 2 * offsetX, y0 + 8 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("VIDEO RECORD [v]"), cv::Point(x0 + 2 * offsetX, y0 + 9 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("ENTER DEVELOPMENT MODE [e]"), cv::Point(x0 + 2 * offsetX, y0 + 10 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);

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
widgets::systemstatus::motor_status_t widgets::systemstatus::motor_status;

void widgets::systemstatus::init()
{
    tegra_status = false;
    att_status = false;
    vid_status = false;
    js_status = false;
    motor_voltage_in = 0.f;
    motor_voltage_out = 0.f;
    duty_cycle = 0.f;
    motor_status = motor_status_t::FAILURE;
}


void widgets::systemstatus::updateCbit(quint32 cbit)
{
    arduino_status = !(cbit & ARDUINO_NODATA);
    att_status = !(cbit & ATTITUDE_NODATA);
    js_status = !(cbit & JOYSTICK_NODATA);
    motor_status = (cbit & MOTORS_NODATA) ? motor_status_t::FAILURE : motor_status;
    tegra_status = !(cbit & TEGRA_NODATA);
    vid_status = !(cbit & VIDEO_NODATA);
}


void widgets::systemstatus::updateMotorVoltageIn(double voltage)
{
    motor_voltage_in = voltage;
    motor_voltage_out = motor_voltage_in * duty_cycle;
}

void widgets::systemstatus::updateMotorVoltageOut(uint8_t pwm)
{
    duty_cycle = (float)pwm / (float)std::numeric_limits<uint8_t>::max();
    motor_voltage_out = motor_voltage_in * duty_cycle;
}

void widgets::systemstatus::updateRemoteSystemState(uint8_t system_state)
{
    motor_status = system_state == 0x00 ? motor_status_t::IDLE :
                                          motor_status_t::RUNNING;
}

void widgets::systemstatus::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[SYSTEM_STATUS])
    {
        int lineSpacing = 20;
        int offsetX = 5;
        int width = 210;
        int height = 9 * (lineSpacing + 2);
        int centerx = x;
        int centery = y;
        int x0 = centerx - width/2;
        int y0 = centery - height/2;

        float battery_charge_percentage = motor_voltage_in / 10.f; //MAX VOLTAGE 10.0 V
        char prompt_motor_voltage_in[64];
        char prompt_motor_voltage_out[64];
        char prompt_motor_status[64];

        sprintf(prompt_motor_voltage_in, "%.1f V", motor_voltage_in);
        sprintf(prompt_motor_voltage_out, "%.1f V", motor_voltage_out);
        sprintf(prompt_motor_status, "MOTORS: %s", motor_status == motor_status_t::IDLE ? "IDLE" :
                                                   motor_status == motor_status_t::RUNNING ? "RUNNING" : "UNKNOWN");

        filledRoundedRectangle(*imagewindow, cv::Point(x0, y0), cv::Size(width, height), bgCol, cv::LINE_AA, 1, 0.1f);
        cv::putText(*imagewindow, cv::String("TEGRA DETECTOR"), cv::Point(x0 + offsetX, y0 + 1 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("LSM9DS1 IMU"), cv::Point(x0 + offsetX, y0 + 2 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("VIDEO CAMERA"), cv::Point(x0 + offsetX, y0 + 3 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("ARDUINO NANO"), cv::Point(x0 +  offsetX, y0 + 4 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("MOTOR BATTERY STATUS"), cv::Point(x0 +  offsetX, y0 + 6 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("MOTOR VOLTAGE IN"), cv::Point(x0 +  offsetX, y0 + 7 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("MOTOR VOLTAGE OUT"), cv::Point(x0 +  offsetX, y0 + 8 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String(prompt_motor_status), cv::Point(x0 + offsetX, y0 + 9 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String("JOYSTICK"), cv::Point(x0 +  offsetX, y0 + 5 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);

        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 1 * lineSpacing - 9), cv::Size(15, 15), tegra_status ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 2 * lineSpacing - 9), cv::Size(15, 15), att_status ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 3 * lineSpacing - 9), cv::Size(15, 15), vid_status ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 4 * lineSpacing - 9), cv::Size(15, 15), arduino_status  ? green : red, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 6 * lineSpacing - 9), cv::Size(15, 15),
                               battery_charge_percentage >= 0.70 ? okBatteryCol :
                               battery_charge_percentage >= 0.40 ? warnBatteryCol : failBatteryCol, cv::LINE_AA, 1, 0.5f);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 9 * lineSpacing - 9), cv::Size(15, 15), motor_status == motor_status_t::RUNNING ? green :
                                                                                                                       motor_status == motor_status_t::IDLE ? yellow : red, cv::LINE_AA, 1, 0.5f);
        cv::putText(*imagewindow, cv::String(prompt_motor_voltage_in), cv::Point(x0 + 30 * offsetX, y0 + 7 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        cv::putText(*imagewindow, cv::String(prompt_motor_voltage_out), cv::Point(x0 + 30 * offsetX, y0 + 8 * lineSpacing), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        filledRoundedRectangle(*imagewindow, cv::Point(x0 + 30 * offsetX, y0 + 5 * lineSpacing - 9), cv::Size(15, 15), js_status  ? green : red, cv::LINE_AA, 1, 0.5f);
    }
}

/** JS HELP WIDGET **/
void widgets::js_help::draw(cv::Mat* imagewindow, int x, int y)
{
    if (is_enabled[JS_HELP])
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

widgets::devmode::DeveloperMode::DeveloperMode()
{
    //act_tab = ATTITUDE;
    //act_tabitem = VIDEOREC_FPS;
}
