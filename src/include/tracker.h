#ifndef TRACKER_H
#define TRACKER_H
#include <opencv2/opencv.hpp>
#include <QThread>
#include <semaphore.h>

typedef enum
{
  IDLE,
  ACQUIRING,
  RUNNING,
  EXITING
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
    void stop();

signals:
    void debugger_new_frame(cv::Mat frame);
    void debugger_track_pattern(cv::Mat frame);
    void region_updated(cv::Rect new_region);
    void thread_quit();

private:
    cv::Mat old, act;
    std::vector<cv::Point> contour_old;
    cv::Rect* region;
    cv::Rect* original_region;
    sem_t image_semaphore;
    sem_t state_semaphore;
    tracker_state_t state;

    void build_hist(cv::Mat grayscale, ulong* hist);
    ulong max_delta(ulong* hist);
    std::vector<cv::Point> estimate_contour(cv::Mat frame);
    cv::Mat shift(cv::Mat &img, int offsetx, int offsety);
    bool equals(cv::Mat a, cv::Mat b, bool* allNotEquals);
};

#endif // TRACKER_H
