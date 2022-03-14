#ifndef WIDGETS_H
#define WIDGETS_H
#include "defs.h"
#include "utils.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <QVariant>
#include <QString>
#include <QMap>

const cv::Scalar blue(255, 0, 0);
const cv::Scalar green(0, 255, 0);
const cv::Scalar red(0, 0, 255);
const cv::Scalar yellow(0, 255, 255);
const cv::Scalar gray(200, 200, 200);
const cv::Scalar black(0, 0, 0);

const cv::Scalar bgCol = blue;
const cv::Scalar fgCol = gray;
const cv::Scalar okCol = green;
const cv::Scalar warnCol = yellow;
const cv::Scalar failCol = red;

const int lineSpacing = 30;
const int fontScale = 5;

class CVMatWidget
{
    public:
        CVMatWidget()
        {
            visible = false;
            this->data_len = 256;
            this->data = new char[this->data_len];
            memset(this->data, 0x00, this->data_len);
        }

        virtual void update(char* data, quint64 data_len)
        {
            if (data_len > this->data_len)
            {
                delete[] this->data;

                this->data = new char[data_len];
                memset(this->data, 0x00, data_len);
            }

            memcpy(this->data, data, data_len);
            this->data_len = data_len;
        }

        void hide()
        {
            visible = false;
        }

        void show()
        {
            visible = true;
        }

        virtual void draw(cv::Mat* frame, cv::Point coord, cv::Size size)
        {
            if (visible)
            {
                filledRoundedRectangle(*frame, coord, size, bgCol, cv::LINE_AA, 1, 0.01f);
            }
        }

    bool enabled()
    {
        return visible;
    }

    protected:
        char* data;
        quint64 data_len;
        bool visible;
};

class TargetWidget : public CVMatWidget
{

public:
    virtual void update(char* data, quint64 data_len);
    virtual void draw(cv::Mat* frame, cv::Point coord = cv::Point(0, 0), cv::Size size = cv::Size(0,0));
private:
    std::vector<cv::Rect> rectangles;
    std::vector<cv::String> rectangle_names;
    std::vector<float> rectangle_confidences;

};

class SpeedometerWidget : public CVMatWidget
{
public:
    virtual void draw(cv::Mat* frame, cv::Point coord, cv::Size size);
};

class MenuCvMatWidget : public CVMatWidget
{

    public:

        struct MenuItem
        {
            QString text;
            bool view_only;
        };

        MenuCvMatWidget(std::vector<MenuItem> items)
        {
            for (auto& item : items)
            {
                this->items.push_back(item);
            }

            vindex = 0;

            max_strlen = 0;
            for(int i = 0; i < int(items.size()); i++)
            {
                if (items[i].text.length() > max_strlen)
                {
                    max_strlen = items[i].text.length();
                }
            }
        }

        virtual void navigateVertical(int delta);
        virtual void draw(cv::Mat* frame, cv::Point coord, cv::Size size = cv::Size(0,0));
        virtual void setItemText(QString text, int item=-1);

        QString getSelectedItem()
        {
            return items[vindex].text;
        }

        int getSelectedIndex()
        {
            return vindex;
        }

    protected:
        int vindex;
        int max_strlen;
        std::vector<MenuItem> items;

};

class SystemMenuWidget : public MenuCvMatWidget
{
public:
    SystemMenuWidget(std::vector<MenuItem> items,
                     int tegra_index,
                     int imu_index,
                     int camera_index,
                     int arduino_index,
                     int joystick_index,
                     int motor_voltage_index,
                     int motor_status_index,
                     int tracker_index

                     ) : MenuCvMatWidget(items)
    {
        vindex = 0;
        while (items[vindex].view_only) vindex += 1;

        if (vindex > int(items.size()) - 1)
        {
            vindex = 0;
        }

        this->tegra_index = tegra_index;
        this->arduino_index = arduino_index;
        this->imu_index = imu_index;
        this->camera_index = camera_index;
        this->motor_voltage_index = motor_voltage_index;
        this->motor_status_index = motor_status_index;
        this->joystick_index = joystick_index;
        this->tracker_index = tracker_index;
        this->comp_status = new bool [items.size()];
        memset(this->comp_status, 0x00, sizeof(bool) * items.size());

        this->voltage_in = 0;
        this->voltage_out = 0;
        this->tracker_status = tracker_state_t::IDLE;
    };


public:
    virtual void update(char* data, quint64 data_len);
    virtual void draw(cv::Mat* frame, cv::Point coord, cv::Size size = cv::Size(0,0));
    void update_voltage(double voltage, double duty_cycle);
    void update_system_status(quint8 system_status);
    void update_tracker_status(tracker_state_t tracker_status);

private:
    int tegra_index;
    int arduino_index;
    int imu_index;
    int camera_index;
    int motor_status_index;
    int motor_voltage_index;
    int joystick_index;
    int tracker_index;

    quint8 system_status;
    double voltage_in;
    double voltage_out;
    tracker_state_t tracker_status;
    bool* comp_status;


    void fillCircleAt(cv::Mat* frame, cv::Point coord, cv::Size size, int index, bool status);
    void drawStringAt(cv::Mat* frame, cv::Point coord, int index, QString string);
};

class PlotWidget : public CVMatWidget
{

};

#endif // WIDGETS_H
