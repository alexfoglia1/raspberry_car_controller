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
        cv::Size rectSize(17 * max_strlen, items.size() * (lineSpacing + 2));

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
    this->comp_status[camera_index] = (cbit & VIDEO_NODATA) == 0;
    this->comp_status[joystick_index] = (cbit & JOYSTICK_NODATA) == 0;
    this->comp_status[imu_index] = (cbit & ATTITUDE_NODATA) == 0;
    this->comp_status[motor_status_index] = (cbit & MOTORS_NODATA) == 0;
}

void SystemMenuWidget::draw(cv::Mat* frame, cv::Point coord, cv::Size size)
{
    if (visible)
    {
        MenuCvMatWidget::draw(frame, coord, size);

        fillCircleAt(frame, coord, cv::Size(15,15), arduino_index, comp_status[arduino_index]);
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
        drawStringAt(frame, coord, raspi_addr_index, QString(board_addr));
    }
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

void SystemMenuWidget::update_board_addr(QString board_addr)
{
    this->board_addr = QString("%1").arg(board_addr);
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
    if (serie >= 0 && serie < int(this->values.size()))
    {
        if (int(this->values[serie].second->size()) == n_values)
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
    Q_UNUSED(scale_x);

    double dy = y_span / 20;
    double offset_y = 0;

    double y = 0;

    while (y < y_span)
    {
        int x = tl.x;
        int dx = size.width / 80;
        while (x < size.width)
        {
            int height_of_y = y * scale_y;
            cv::line(*frame, cv::Point(x, size.height + tl.y - height_of_y - offset_y), cv::Point(x + dx, size.height + tl.y - height_of_y - offset_y), fgCol, 1, cv::LINE_AA);
            x += 2 * dx;
        }
        cv::String y_str(QString("%1").arg(y).toStdString().c_str());
        cv::putText(*frame, cv::String(y_str), cv::Point(tl.x, size.height + tl.y - y * scale_y - offset_y), cv::FONT_HERSHEY_SIMPLEX, 0.35, fgCol, 1, cv::LINE_AA);
        y += dy;
    }
}

void PlotWidget::draw_legend(cv::Mat *frame, cv::Size size, cv::Point coord)
{
    cv::Point legend_coord = cv::Point(coord.x + 8 * size.width / 10, coord.y + 10);
    cv::Size legend_size = cv::Size(77, 80);
    filledRoundedRectangle(*frame, legend_coord, legend_size, cv::Scalar(0, 0, 0), cv::LINE_AA, 1, 0.01);
    double dy = 25;
    for (int i = 0; i < int(values.size()); i++)
    {
        auto& pair = values.at(i);
        cv::putText(*frame, pair.first, cv::Point(legend_coord.x, legend_coord.y + (i + 1) * dy), cv::FONT_HERSHEY_SIMPLEX, 0.5, colors.at(i % colors.size()), 1, cv::LINE_AA);
    }

}

void PlotWidget::draw(cv::Mat *frame, cv::Point coord, cv::Size size)
{
    if (visible)
    {
        CVMatWidget::draw(frame, coord, size);
        double offset_y = 0;
        double scale_x = (double)size.width / (double)n_values;
        double scale_y = (double)(size.height - offset_y) / (double)y_span;


        draw_axis(frame, coord, size, scale_x, scale_y);
        draw_legend(frame, size, coord);

        for(int pair_idx = 0; pair_idx < int(values.size()); pair_idx++)
        {
            auto& pair = values.at(pair_idx);
            for (int i = 1; i < int(pair.second->size()); i++)
            {
                double value_prev = (i - 1) < int(pair.second->size()) ? pair.second->at(i - 1) : 0.0;
                double value = i < int(pair.second->size()) ? pair.second->at(i) : 0.0;
                int x_coord_prev = coord.x + (i - 1) * scale_x;
                int x_coord = coord.x + i * scale_x;
                int y_coord_prev = size.height + coord.y - value_prev * scale_y - offset_y;
                int y_coord = size.height + coord.y - value * scale_y - offset_y;

                cv::line(*frame, cv::Point(x_coord_prev, y_coord_prev), cv::Point(x_coord, y_coord), colors.at(pair_idx % int(colors.size())), 1, cv::LINE_AA);
            }
        }
    }
}

void LosWidget::update(char* data, quint64 data_len)
{
    attitude_msg msg = *reinterpret_cast<attitude_msg*>(data);

    CVMatWidget::update((char*)&msg, data_len);
}

void LosWidget::draw(cv::Mat*frame, cv::Point coord, cv::Size size)
{
    if (visible)
    {
        attitude_msg data = *reinterpret_cast<attitude_msg*>(this->data);
        double act_yaw_deg = toDegrees(data.yaw);
        double act_pitch_deg = toDegrees(data.pitch);
        double act_roll_deg = toDegrees(data.roll);

        int recwidth = size.width;
        int recheight = size.height;
        const int LOS_RAY = std::min<int>(recwidth / 2, recheight / 2);
        const int H_FOV_DEG = 53;

        cv::Point losCenter = cv::Point(coord.x + LOS_RAY, coord.y + LOS_RAY);

        int r_brown = 115;
        int g_brown = 51;
        int b_brown = 8;

        filledRoundedRectangle(*frame, cv::Point(losCenter.x - LOS_RAY, losCenter.y - LOS_RAY), cv::Size(recwidth, recheight), bgCol, cv::LINE_AA, 1, 0.001f);
        filledRoundedRectangle(*frame, cv::Point(losCenter.x - LOS_RAY, losCenter.y), cv::Size(recwidth, recheight / 2), cv::Scalar(b_brown, g_brown, r_brown), cv::LINE_AA, 1, 0.001f);

        double left_bound_deg = normalizeAngle(toRadians((act_yaw_deg - H_FOV_DEG / 2)));
        double right_bound_deg = normalizeAngle(toRadians((act_yaw_deg + H_FOV_DEG / 2)));
        double pitch90 = -asin(sin((atan2(sin(act_pitch_deg * 3.14/180.0), cos(act_pitch_deg * 3.14/180.0)) * 180.0/3.14) * 3.14/180.0)) * 180.0/3.14;
        double elevPercentage = pitch90 / 90;

        cv::Point2d losLeft(losCenter.x + LOS_RAY * cos(toRadians(left_bound_deg - 90)), losCenter.y + LOS_RAY * sin(toRadians(left_bound_deg - 90)));
        cv::Point2d losRight(losCenter.x + LOS_RAY * cos(toRadians(right_bound_deg - 90)), losCenter.y + LOS_RAY * sin(toRadians(right_bound_deg - 90)));
        cv::Point2d losElev(losCenter.x, losCenter.y + elevPercentage * LOS_RAY);
        cv::Point2d losRollRight(losCenter.x + (2 * LOS_RAY / 3) * cos(toRadians(act_roll_deg)), losCenter.y + (2 * LOS_RAY / 3) * sin(toRadians(act_roll_deg)));
        cv::Point2d losRollLeft(losCenter.x - (2 * LOS_RAY / 3) * cos(toRadians(act_roll_deg)), losCenter.y - (2 * LOS_RAY / 3) * sin(toRadians(act_roll_deg)));

        /** Plot actual attitude **/
        cv::line(*frame, losCenter, losLeft, green, 1);
        cv::line(*frame, losCenter, losRight, green, 1);
        cv::line(*frame, losRollLeft, losRollRight, green, 1);
        cv::drawMarker(*frame, losElev, green, cv::MARKER_CROSS, 20, 1, cv::LINE_AA);

        /** Scales **/
        /** External ellipse: for yaw **/
        cv::ellipse(*frame, losCenter, cv::Size(LOS_RAY, LOS_RAY), 15, 0, 360, fgCol, 1);
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

            cv::line(*frame, notch_start, notch_end, fgCol, 1);
            cv::putText(*frame, cv::String(buf), cv::Point2d(txt_x, txt_y), cv::FONT_HERSHEY_SIMPLEX, 0.25, fgCol, 1, cv::LINE_AA);

            grad_scale_yaw_deg += 45.0/2;
        }
        /** Internal ellipse: for roll **/
        cv::ellipse(*frame, losCenter, cv::Size(2 * LOS_RAY / 3, 2 * LOS_RAY / 3), 15, 0, 360, fgCol, 1);
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

            cv::line(*frame, notch_start, notch_end, fgCol, 1);
            cv::putText(*frame, cv::String(buf), cv::Point2d(txt_x, txt_y), cv::FONT_HERSHEY_SIMPLEX, 0.25, fgCol, 1, cv::LINE_AA);

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
            cv::line(*frame, notch_start, notch_end, fgCol, 1);

            grad_scale_pitch_deg += 45.0/2;
        }
     }
}
