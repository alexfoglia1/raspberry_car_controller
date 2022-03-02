#ifndef VIDEO_ALGO_H
#define VIDEO_ALGO_H

#include <opencv2/opencv.hpp>

void clahe(cv::Mat* image);
void polarity(cv::Mat* image);
void channel_filter(cv::Mat* image, int channel_index);

#endif //VIDEO_ALGO_H
