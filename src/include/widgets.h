#ifndef WIDGETS_H
#define WIDGETS_H
#include "defs.h"
#include "utils.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <QVariant>
#include <QString>

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
        }

        void update(char* data)
        {
            this->data = data;
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
        bool visible;
};

class MenuCvMatWidget : public CVMatWidget
{
    public:

        MenuCvMatWidget(std::vector<QString> items)
        {
            for (auto& item : items)
            {
                this->items.push_back(item);
            }

            vindex = 0;
        }

        virtual void navigateVertical(int delta)
        {
            if (visible)
            {
                vindex += delta;

                if (vindex < 0)
                {
                    vindex = items.size() - 1;
                }

                if (vindex > items.size() - 1)
                {
                    vindex = 0;
                }
            }
        }

        virtual void draw(cv::Mat* frame, cv::Point coord, cv::Size size = cv::Size(0,0))
        {
            Q_UNUSED(size);

            if (visible)
            {
                int max_strlen = 0;
                for(int i = 0; i < items.size(); i++)
                {
                    if (items[i].length() > max_strlen)
                    {
                        max_strlen = items[i].length();
                    }
                }
                cv::Size rectSize(10 * max_strlen, items.size() * (lineSpacing + 2));

                CVMatWidget::draw(frame, coord, rectSize);

                if (vindex >= 0)
                {
                    cv::Rect selection(coord.x + lineSpacing/2, lineSpacing/2 + coord.y + vindex * lineSpacing, 7 * max_strlen, lineSpacing / 2);
                    filledRoundedRectangle(*frame, selection.tl(), selection.size(), fgCol, cv::LINE_AA, 1, 0.01f);
                }
                for (int i = 0; i < int(items.size()); i++)
                {
                    cv::Point act_coord((coord.x + lineSpacing/2), coord.y + (i + 1) * lineSpacing - 1);
                    cv::putText(*frame, cv::String(items[i].toStdString()), act_coord, cv::FONT_HERSHEY_SIMPLEX, 0.35, (i == vindex) ? bgCol : fgCol, 1, cv::LINE_AA);
                }
            }
        }

        QString getSelectedItem()
        {
            return items[vindex];
        }

        int getSelectedIndex()
        {
            return vindex;
        }

    protected:
        int vindex;
        std::vector<QString> items;

};

class SystemStatus : public MenuCvMatWidget
{
    void navigateVertical(int delta)
    {
        Q_UNUSED(delta);
    }


};

#endif // WIDGETS_H
