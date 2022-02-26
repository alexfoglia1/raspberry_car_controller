#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include "widgets.h"
#include "defs.h"

#include <semaphore.h>
#include <opencv2/opencv.hpp>
#include <QThread>


class VideoRenderer : public QThread
{
    Q_OBJECT
public:
    static const int RENDER_FREQ_HZ = 30;
    double render_timeout_micros = 1e6/RENDER_FREQ_HZ;

    VideoRenderer();
    void init_window();

protected:
    void run() override;

public slots:
    /** Remote controlled slots **/
    void update(image_msg image);
    void update(voltage_msg voltage);
    void update(attitude_msg attitude);
    void update(actuators_state_msg actuators);
    void update(target_msg targets);

    /** Local controlled slots **/
    void toggle_widget(bool enabled, int widget);
    void toggle_videorec(int fps);

private:

    cv::Mat* imagewindow;
    sem_t image_semaphore;
    cv::VideoWriter* video;

    bool save_frame;
    bool stopped;

    void render_window();
    void start_videorec(int fps);

};

#endif //VIDEO_RENDERER_H
