#ifndef VIDEO_ALGO_H
#define VIDEO_ALGO_H

#include <opencv2/opencv.hpp>
#include <QObject>

class VideoProcessor : public QObject
{
    Q_OBJECT

public:
    typedef struct
    {
        cv::Mat frame_2;
        cv::Mat frame_1;
        cv::Mat frame_0;
        int available_frames;
    } vidpro_history_t;

    VideoProcessor();

private:
    vidpro_history_t history;
    bool clahe_enabled;
    bool polarity_enabled;
    bool filter_enabled[3];
    bool denoise_enabled;

    void do_clahe();
    void do_polarity();
    void do_channel_filter(int channel_index);
    void do_denoise();

public slots:
    void feed(cv::Mat image);
    void clahe(bool enabled);
    void polarity(bool enabled);
    void channel_filter(bool enabled, int channel_index);
    void denoise(bool enabled);

signals:
    void frame_ready(cv::Mat frame);
};



#endif //VIDEO_ALGO_H
