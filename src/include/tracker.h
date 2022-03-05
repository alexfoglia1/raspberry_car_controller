#ifndef TRACKER_H
#define TRACKER_H
#include <opencv2/opencv.hpp>
#include <QObject>

class Tracker : public QObject
{
    Q_OBJECT

public:
    Tracker();
    cv::Mat correlate(cv::Mat frame1, cv::Mat frame2, cv::Rect region);

public slots:
    void on_camera_image(cv::Mat frame_from_camera);

private:
    cv::Mat old, act;
    int rx;
};

#endif // TRACKER_H
