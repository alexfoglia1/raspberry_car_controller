#include "widgets.h"
#include "utils.h"
#include "cbit.h"

void SpeedometerWidget::draw(cv::Mat* frame, cv::Point coord, cv::Size size)
{
    CVMatWidget::draw(frame, coord, size);
    if (visible)
    {
        quint8 throttle_state = data == nullptr ? 0x00 : *(quint8*)data;

        char throttle_display[100];
        memset(throttle_display, ' ', 99);
        throttle_display[99] = '\0';

        quint8 perc = ((double)throttle_state / 255.0) * 100.0;

        for(quint8 i = 0; i < 99; i++)
        {
            throttle_display[i] = (i < perc) ? '*' : ' ';
        }

        cv::putText(*frame, cv::String(QString("[%1]").arg(throttle_display).toStdString()), cv::Point(coord.x + 10, coord.y + 10), cv::FONT_HERSHEY_SIMPLEX, 0.175, fgCol, 1, cv::LINE_AA);
    }
}


void TargetWidget::update(char *data, quint64 data_len)
{
    CVMatWidget::update(data, data_len);

    rectangles.clear();
    rectangle_confidences.clear();
    rectangle_names.clear();

    target_msg targets = *(target_msg*) data;
    for (quint8 i = 0; i < targets.n_targets; i++)
    {
        rectangles.push_back(cv::Rect(targets.data[i].x_pos, targets.data[i].y_pos, targets.data[i].width, targets.data[i].height));
        rectangle_confidences.push_back(targets.data[i].confidence);
        rectangle_names.push_back(targets.data[i].description);
    }
}

void TargetWidget::draw(cv::Mat* frame, cv::Point coord, cv::Size size)
{
    Q_UNUSED(coord)
    Q_UNUSED(size)

    if (visible)
    {
        for (int i = 0; i < int(rectangles.size()); i++)
        {
            auto& rectangle = rectangles[i];
            cv::rectangle(*frame, rectangle, cv::Scalar(255, 255, 0), 1, cv::LINE_AA);
            QString text = QString("%1 (%2)").arg(rectangle_names[i].c_str()).arg(rectangle_confidences[i]);
            cv::putText(*frame, cv::String(text.toStdString()), cv::Point(rectangle.x + 4.0 * rectangle.width/5.0, rectangle.y + 20), cv::FONT_HERSHEY_SIMPLEX, 0.4, bgCol, 1, cv::LINE_AA);
        }
    }
}

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

void MenuCvMatWidget::setItemText(QString text, int item)
{
    int _item = item == -1 || item > int(items.size()) ? vindex : item;
    items[_item].text = text;
}


void MenuCvMatWidget::draw(cv::Mat* frame, cv::Point coord, cv::Size size)
{
    Q_UNUSED(size);

    if (visible)
    {
        cv::Size rectSize(15 * max_strlen, items.size() * (lineSpacing + 2));

        CVMatWidget::draw(frame, coord, rectSize);

        if (vindex >= 0 && !items[vindex].view_only)
        {
            cv::Rect selection(coord.x + lineSpacing/2, lineSpacing/2 + coord.y + vindex * lineSpacing, 7 * items[vindex].text.length(), lineSpacing / 2);
            filledRoundedRectangle(*frame, selection.tl(), selection.size(), fgCol, cv::LINE_AA, 1, 0.01f);
        }

        for (int i = 0; i < int(items.size()); i++)
        {
            cv::Point act_coord((coord.x + lineSpacing/2), coord.y + (i + 1) * lineSpacing - 1);
            cv::putText(*frame, cv::String(items[i].text.toStdString()), act_coord, cv::FONT_HERSHEY_SIMPLEX, 0.35, (i == vindex && !items[vindex].view_only) ? bgCol : fgCol, 1, cv::LINE_AA);
        }
    }
}

void SystemMenuWidget::update(char* data, quint64 data_len)
{
    CVMatWidget::update(data, data_len);

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

    float f_rounded_voltage_in = int(10 * voltage_in) / 10.f;
    float f_rounded_voltage_out = int(10 * voltage_out) / 10.f;

    QString rounded_voltage_in = f_rounded_voltage_in == int(f_rounded_voltage_in) ? QString("%1.0").arg(f_rounded_voltage_in) : QString("%1").arg(f_rounded_voltage_in);
    QString rounded_voltage_out = f_rounded_voltage_out == int(f_rounded_voltage_out) ? QString("%1.0").arg(f_rounded_voltage_out) : QString("%1").arg(f_rounded_voltage_out);
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

    cv::putText(*frame, cv::String(string.toStdString()), cv::Point(coord.x + offset_x, coord.y + offset_y), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
}

int PlotWidget::create_new_serie(cv::String displayName, cv::Scalar color)
{
    this->values.push_back(std::pair<cv::String, std::vector<double>*>(displayName, new std::vector<double>));
    this->colors.push_back(color);

    return this->values.size() - 1;
}

void PlotWidget::update_serie(int serie, double value)
{
    if (serie >= 0 && serie < this->values.size())
    {
        if (this->values[serie].second->size() == n_values)
        {
            for (int i = 0; i < int(this->values[serie].second->size() - 1); i++)
            {
                this->values[serie].second->at(i) = this->values[serie].second->at(i + 1);
            }
            this->values[serie].second->at(n_values - 1) = value;
        }
        else
        {
            this->values[serie].second->push_back(value);
        }
    }
}

void PlotWidget::draw_axis(cv::Mat *frame, cv::Point tl, cv::Size size, double scale_x, double scale_y)
{
    double dy = (y_max - y_min) / 20;
    double y = y_max;
    double half_screen_value = (y_max - y_min) / 2;
    double half_screen_height = size.height / 2;

    cv::line(*frame, cv::Point(tl.x, tl.y + height_of_y0), cv::Point(tl.x + size.width, tl.y + height_of_y0), fgCol, 1, cv::LINE_AA);

    while (y >= y_min)
    {
        int x = tl.x;
        int dx = size.width / 80;
        while (x < size.width)
        {
            cv::line(*frame, cv::Point(x, tl.y + height_of_y0 - (y * scale_y)), cv::Point(x + dx, tl.y + height_of_y0 - (y * scale_y)), fgCol, 1, cv::LINE_AA);
            x += 2 * dx;
        }
        y -= dy;
    }
}

void PlotWidget::draw(cv::Mat *frame, cv::Point coord, cv::Size size)
{
    if (visible)
    {
        CVMatWidget::draw(frame, coord, size);

        double scale_x = (double)size.width / (double)n_values;
        double scale_y = (double)size.height / (y_max - y_min);
        double y0 = y_min;
        int height_of_y0 = y0 * scale_y;

        draw_axis(frame, coord, size, scale_x, scale_y);

        for(int pair_idx = 0; pair_idx < int(values.size()); pair_idx++)
        {
            auto& pair = values.at(pair_idx);
            for (int i = 1; i < n_values; i++)
            {
                double value_prev = (i - 1) < pair.second->size() ? pair.second->at(i - 1) : 0.0;
                double value = i < pair.second->size() ? pair.second->at(i) : 0.0;
                int x_coord_prev = coord.x + (i - 1) * scale_x;
                int x_coord = coord.x + i * scale_x;
                int y_coord_prev = coord.y + height_of_y0 - value_prev * scale_y;
                int y_coord = coord.y + height_of_y0 - value * scale_y;

                cv::line(*frame, cv::Point(x_coord_prev, y_coord_prev), cv::Point(x_coord, y_coord), colors.at(pair_idx % int(colors.size())), 1, cv::LINE_AA);
            }
        }
    }
}
