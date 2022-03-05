#include "tracker.h"

#include <math.h>
#include "video_renderer.h"


Tracker::Tracker(cv::Rect region)
{
    rx = 0;
    this->region = new cv::Rect(region.x, region.y, region.width, region.height);
    sem_init(&image_semaphore, 0, 1);
}

void Tracker::on_camera_image(cv::Mat frame_from_camera)
{
    cv::Mat copy = frame_from_camera;
    cv::Mat* image = new cv::Mat(copy.size(), copy.type());

    sem_wait(&image_semaphore);
    if (rx == 0)
    {
        memcpy(image->data, copy.data, copy.dataend - copy.data);
        old = *image;
        rx = 1;

    }
    else if (rx == 1)
    {
        memcpy(image->data, copy.data, copy.dataend - copy.data);
        act = *image;
        rx = 2;
    }
    else
    {
        memcpy(image->data, copy.data, copy.dataend - copy.data);

        old = act;
        act = *image;
        rx = 3;
    }
    delete image;
    sem_post(&image_semaphore);
}

void build_hist(cv::Mat grayscale, ulong* hist)
{
    memset(hist, 0x00, 0xFF * sizeof(ulong));

    cv::Size mat_size = grayscale.size();
    int mat_data_len = mat_size.area();
    for (uint i = 0; i < mat_data_len; i++)
    {
        uchar gval = grayscale.data[i];
        hist[gval] += 1;
    }
}

ulong max_delta(ulong* hist)
{
    double max = 0;
    for(uint i = 0; i < 0xFF; i++)
    {
        for(uint j = 0; j < 0xFF; j++)
        {
            double delta = fabs(double(hist[i]) - double(hist[j]));
            if (delta > max)
            {
                max = delta;
            }
        }
    }

    return ulong(max);
}

void Tracker::run()
{
                GLViewer* viewer = new GLViewer();
    while (true)
    {
        if (rx > 2)
        {
            sem_wait(&image_semaphore);

            cv::Mat grey_old, grey_act;

            cv::cvtColor(old(*region), grey_old, cv::COLOR_BGR2GRAY);
            cv::cvtColor(act(*region), grey_act, cv::COLOR_BGR2GRAY);

            sem_post(&image_semaphore);

            ulong hist_old[0xFF];
            ulong hist_act[0xFF];

            build_hist(grey_old, hist_old);
            build_hist(grey_act, hist_act);

            int max_old = max_delta(hist_old);
            int max_act = max_delta(hist_act);

            int num_rows = grey_old.size().height;
            int num_cols = grey_old.size().width;

            const double SIMILARITY_THRESHOLD = 0.90; //da parametrizzare
            std::vector<cv::Point> contour;

            /** Left to right scan **/
            for (int y = 0; y < num_rows; y++)
            {
                uchar p0 = grey_old.data[y * num_cols];
                std::vector<std::pair<cv::Point, double>> contour_candidates_wrt_p1;
                for (int x = 0; x < num_cols; x++)
                {
                    uchar p1 = grey_old.data[y * num_cols + x];

                    double act_delta = fabs(double(hist_old[p1]) - double(hist_old[p0]));
                    double ratio = act_delta / double(max_old);

                    if (ratio >= SIMILARITY_THRESHOLD)
                    {
                        contour_candidates_wrt_p1.push_back(std::pair<cv::Point, double>(cv::Point(x, y), ratio));
                    }
                }

                double max_ratio = 0;
                cv::Point max_ratio_point;
                for (auto& pair : contour_candidates_wrt_p1)
                {
                    if (pair.second > max_ratio)
                    {
                        max_ratio = pair.second;
                        max_ratio_point = pair.first;
                    }
                }

                contour.push_back(max_ratio_point);
            }

            /** Right to left scan **/
            for (int y = 0; y < num_rows; y++)
            {
                uchar p0 = grey_old.data[y * num_cols];
                std::vector<std::pair<cv::Point, double>> contour_candidates_wrt_p1;
                for (int x = num_cols - 1; x >= 0; x--)
                {
                    uchar p1 = grey_old.data[y * num_cols + x];

                    double act_delta = fabs(double(hist_old[p1]) - double(hist_old[p0]));
                    double ratio = act_delta / double(max_old);

                    if (ratio >= SIMILARITY_THRESHOLD)
                    {
                        contour_candidates_wrt_p1.push_back(std::pair<cv::Point, double>(cv::Point(x, y), ratio));
                    }
                }

                double max_ratio = 0;
                cv::Point max_ratio_point;
                for (auto& pair : contour_candidates_wrt_p1)
                {
                    if (pair.second > max_ratio)
                    {
                        max_ratio = pair.second;
                        max_ratio_point = pair.first;
                    }
                }

                contour.push_back(max_ratio_point);
            }

            cv::Mat contour_mat(num_rows, num_cols, CV_8UC1, cv::Scalar(0));
            for (auto& pt : contour)
            {
                contour_mat.data[pt.y * num_rows + pt.x] = 0xFF;
            }


            emit debugger_frame(contour_mat);
            msleep(100);

        }
    }
}
