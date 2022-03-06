#include "tracker.h"

#include <math.h>
#include "video_renderer.h"


Tracker::Tracker(cv::Rect region)
{
    rx = 0;
    this->region = new cv::Rect(region.x, region.y, region.width, region.height);
    this->state = tracker_state_t::IDLE;
    sem_init(&image_semaphore, 0, 1);
}

void Tracker::on_camera_image(cv::Mat frame_from_camera)
{
    cv::Mat copy = frame_from_camera;
    cv::Mat* image = new cv::Mat(copy.size(), copy.type());

    sem_wait(&image_semaphore);
    if (tracker_state_t::ACQUIRING == state)
    {
        memcpy(image->data, copy.data, copy.dataend - copy.data);
        old = *image;
        state = tracker_state_t::RUNNING;
        printf("tracker running!\n");
    }
    else
    {
        memcpy(image->data, copy.data, copy.dataend - copy.data);
        act = *image;
    }
    delete image;
    sem_post(&image_semaphore);
}

void Tracker::on_change_state()
{
    sem_wait(&image_semaphore);
    if (tracker_state_t::RUNNING == state)
    {
        printf("tracker idle!\n");
        state = tracker_state_t::IDLE;
    }
    else
    {
        printf("tracker acq!\n");
        state = tracker_state_t::ACQUIRING;
    }
}

void build_hist(cv::Mat grayscale, ulong* hist)
{
    memset(hist, 0x00, 0xFF * sizeof(ulong));

    cv::Size mat_size = grayscale.size();
    int mat_data_len = mat_size.area();
    for (int i = 0; i < mat_data_len; i++)
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

cv::Mat estimate_contour(cv::Mat frame)
{
    ulong hist[255];
    build_hist(frame, hist);

    int max_old = max_delta(hist);
    int num_rows = frame.size().height;
    int num_cols = frame.size().width;

    const double SIMILARITY_THRESHOLD = 0.90; //da parametrizzare
    std::vector<cv::Point> contour;

    /** Left to right scan **/
    for (int y = 0; y < num_rows; y++)
    {
        uchar p0 = frame.data[y * num_cols];
        std::vector<std::pair<cv::Point, double>> contour_candidates_wrt_p1;
        for (int x = 0; x < num_cols; x++)
        {
            uchar p1 = frame.data[y * num_cols + x];

            double act_delta = fabs(double(hist[p1]) - double(hist[p0]));
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
        uchar p0 = frame.data[y * num_cols];
        std::vector<std::pair<cv::Point, double>> contour_candidates_wrt_p1;
        for (int x = num_cols - 1; x >= 0; x--)
        {
            uchar p1 = frame.data[y * num_cols + x];

            double act_delta = fabs(double(hist[p1]) - double(hist[p0]));
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

    /** Top to bottom scan **/
    for (int x = 0; x < num_cols; x++)
    {
        uchar p0 = frame.data[x * num_cols];
        std::vector<std::pair<cv::Point, double>> contour_candidates_wrt_p1;
        for (int y = 0; y < num_rows; y++)
        {
            uchar p1 = frame.data[y * num_cols + x];

            double act_delta = fabs(double(hist[p1]) - double(hist[p0]));
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

    /** Bottom to top scan **/
    for (int x = 0; x < num_cols; x++)
    {
        uchar p0 = frame.data[x * num_cols];
        std::vector<std::pair<cv::Point, double>> contour_candidates_wrt_p1;
        for (int y = num_rows - 1; y >= 0; y--)
        {
            uchar p1 = frame.data[y * num_cols + x];

            double act_delta = fabs(double(hist[p1]) - double(hist[p0]));
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

    return contour_mat;
}

cv::Mat shift(cv::Mat &img, int offsetx, int offsety)
{
    cv::Mat trans_mat = (cv::Mat_<double>(2,3) << 1, 0, offsetx, 0, 1, offsety);
    warpAffine(img,img,trans_mat,img.size());
    return img;
}

bool equals(cv::Mat a, cv::Mat b)
{
    cv::Mat diff = a != b;
    return cv::countNonZero(diff) < 200;
}

#include <easy/profiler.h>
void Tracker::run()
{

    while (true)
    {

        if (tracker_state_t::RUNNING == state)
        {
            sem_wait(&image_semaphore);

            cv::Mat grey_old, grey_act;

            cv::cvtColor(old(*region), grey_old, cv::COLOR_BGR2GRAY);
            cv::cvtColor(act(*region), grey_act, cv::COLOR_BGR2GRAY);

            sem_post(&image_semaphore);
            EASY_BLOCK("contour old");
            cv::Mat contour_old = estimate_contour(grey_old);
            EASY_END_BLOCK;

            EASY_BLOCK("contour act");
            cv::Mat contour_act = estimate_contour(grey_act);
            EASY_END_BLOCK;

            bool found = false;
            int x_mov = 0;
            int y_mov = 0;
            int act_shift_amount = 1;
            int spiral_state = 0;
            emit debugger_frame(contour_act);

            while (x_mov < region->width && y_mov < region->height && !found)
            {
                cv::Mat shift_old(contour_old.rows, contour_old.cols, contour_old.type());
                memcpy(shift_old.data, contour_old.data, contour_old.dataend - contour_old.data);
                shift(shift_old, x_mov, y_mov);
                //emit region_updated(cv::Rect(region->x + x_mov, region->y + y_mov, region->width, region->height));
                //
                //        msleep(500);
                if (!equals(shift_old, contour_act))
                {
                    switch (spiral_state)
                    {
                    case 0: /* right of shift amount */
                    {
                        x_mov = act_shift_amount;
                        spiral_state = 1;
                    }
                    break;
                    case 1: /* down of shift amount */
                    {
                        y_mov = act_shift_amount;
                        spiral_state = 2;
                    }
                    break;
                    case 2: /* left of (1 + shift amount) */
                    {
                        act_shift_amount += 1;
                        x_mov = -1 * act_shift_amount;
                        spiral_state = 3;
                    }
                    break;
                    case 3: /* up of shift_amount */
                    {
                        y_mov = -1 * act_shift_amount;
                        spiral_state = 0;
                    }
                    break;

                    }

                }
                else
                {
                    found = true;
                }

            }

            if (found && (x_mov != 0 || y_mov != 0))
            {
                printf("tracker::run():  dx(%d), dy(%d)\n", x_mov, y_mov);
                int new_region_x = region->x + x_mov;
                int new_region_y = region->y + y_mov;
                int new_region_width = region->width;
                int new_region_height = region->height;

                delete region;
                region = new cv::Rect(new_region_x, new_region_y, new_region_width, new_region_height);
                emit region_updated(*region);
            }

        }
    }
}
