#include "widgets.h"
#include "utils.h"
#include "cbit.h"

void MenuCvMatWidget::navigateVertical(int delta)
{
    if (visible)
    {
        vindex += delta;

        if (vindex < 0)
        {
            vindex = items.size() - 1;
        }

        if (vindex > int(items.size()) - 1)
        {
            vindex = 0;
        }

        while (items[vindex].view_only && vindex < int(items.size()))
        {
            vindex += delta > 0 ? 1 : -1;
        }

        if (vindex == int(items.size()))
        {
            vindex = 0;
        }

    }
}


void MenuCvMatWidget::draw(cv::Mat* frame, cv::Point coord, cv::Size size)
{
    Q_UNUSED(size);

    if (visible)
    {
        int max_strlen = 0;
        for(int i = 0; i < int(items.size()); i++)
        {
            if (items[i].text.length() > max_strlen)
            {
                max_strlen = items[i].text.length();
            }
        }
        cv::Size rectSize(15 * max_strlen, items.size() * (lineSpacing + 2));

        CVMatWidget::draw(frame, coord, rectSize);

        if (vindex >= 0 && !items[vindex].view_only)
        {
            cv::Rect selection(coord.x + lineSpacing/2, lineSpacing/2 + coord.y + vindex * lineSpacing, 7 * max_strlen, lineSpacing / 2);
            filledRoundedRectangle(*frame, selection.tl(), selection.size(), fgCol, cv::LINE_AA, 1, 0.01f);
        }

        for (int i = 0; i < int(items.size()); i++)
        {
            cv::Point act_coord((coord.x + lineSpacing/2), coord.y + (i + 1) * lineSpacing - 1);
            cv::putText(*frame, cv::String(items[i].text.toStdString()), act_coord, cv::FONT_HERSHEY_SIMPLEX, 0.35, (i == vindex && !items[vindex].view_only) ? bgCol : fgCol, 1, cv::LINE_AA);
        }
    }
}

void SystemMenuWidget::update(char* data)
{
    CVMatWidget::update(data);

    quint32 cbit = *(quint32*)data;
    this->comp_status[arduino_index] = (cbit & ARDUINO_NODATA) == 0;
    this->comp_status[tegra_index] = (cbit & TEGRA_NODATA) == 0;
    this->comp_status[camera_index] = (cbit & VIDEO_NODATA) == 0;
    this->comp_status[joystick_index] = (cbit & JOYSTICK_NODATA) == 0;
    this->comp_status[imu_index] = (cbit & ATTITUDE_NODATA) == 0;
    this->comp_status[motor_status_index] = (cbit & MOTORS_NODATA) == 0;
}

void SystemMenuWidget::draw(cv::Mat* frame, cv::Point coord, cv::Size size)
{
    MenuCvMatWidget::draw(frame, coord, size);

    fillCircleAt(frame, coord, cv::Size(15,15), arduino_index, comp_status[arduino_index]);
    fillCircleAt(frame, coord, cv::Size(15,15), tegra_index, comp_status[tegra_index]);
    fillCircleAt(frame, coord, cv::Size(15,15), imu_index, comp_status[imu_index]);
    fillCircleAt(frame, coord, cv::Size(15,15), camera_index, comp_status[camera_index]);
    fillCircleAt(frame, coord, cv::Size(15,15), joystick_index, comp_status[joystick_index]);

    drawStringAt(frame, coord, motor_status_index, comp_status[motor_status_index] == false ? "UNKNOWN":
                                                                                                                 system_status == 0x00 ? "IDLE" :
                                                                                                                 system_status == 0x01 ? "RUNNING" : "VALUE ERROR");
    float rounded_voltage_in = int(10 * voltage_in) / 10.f;
    float rounded_voltage_out = int(10 * voltage_out) / 10.f;
    drawStringAt(frame, coord, motor_voltage_index, QString("IN: %1 OUT: %2").arg(rounded_voltage_in).arg(rounded_voltage_out));
}

void SystemMenuWidget::update_voltage(double voltage, double duty_cycle)
{
    this->voltage_in = voltage;
    this->voltage_out = voltage * duty_cycle;
}

void SystemMenuWidget::update_system_status(quint8 system_status)
{
    this->system_status = system_status;
}

void SystemMenuWidget::fillCircleAt(cv::Mat *frame, cv::Point coord, cv::Size size, int index, bool status)
{
    int offset_x = 170;
    int offset_y = 20 + index * lineSpacing;
    filledRoundedRectangle(*frame, cv::Point(coord.x + offset_x, coord.y + offset_y), size, status ? green : red, cv::LINE_AA, 1, 0.5f);
}

void SystemMenuWidget::drawStringAt(cv::Mat *frame, cv::Point coord, int index, QString string)
{
    int offset_x = 100;
    int offset_y =  (index + 1) * lineSpacing - 1;

    cv::putText(*frame, cv::String(string.toStdString()), cv::Point(coord.x + offset_x, coord.y + offset_y), cv::FONT_HERSHEY_SIMPLEX, 0.35, items[index].view_only ? fgCol : index == vindex ? bgCol: fgCol, 1, cv::LINE_AA);
}
