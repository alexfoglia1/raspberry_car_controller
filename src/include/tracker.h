#ifndef TRACKER_H
#define TRACKER_H
#include <opencv2/opencv.hpp>
#include <QThread>
#include <semaphore.h>

typedef enum
{
  IDLE,
  ACQUIRING,
  RUNNING
} tracker_state_t;

class Tracker : public QThread
{
    Q_OBJECT


public:
    Tracker(cv::Rect region);

protected:
    void run();

public slots:
    void on_camera_image(cv::Mat frame_from_camera);
    void on_change_state();

signals:
    void debugger_frame(cv::Mat frame);
    void region_updated(cv::Rect new_region);

private:
    cv::Mat old, act;
    cv::Rect* region;
    int rx;
    sem_t image_semaphore;
    tracker_state_t state;
};

#endif // TRACKER_H
