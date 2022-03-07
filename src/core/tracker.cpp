#include "tracker.h"

#include <math.h>
#include "video_renderer.h"


Tracker::Tracker(cv::Rect region)
{
    this->region = new cv::Rect(region.x, region.y, region.width, region.height);
    this->original_region = new cv::Rect(region.x, region.y, region.width, region.height);
    this->state = tracker_state_t::IDLE;
    sem_init(&image_semaphore, 0, 1);
    sem_init(&state_semaphore, 0, 1);
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
        sem_wait(&state_semaphore);
        cv::Mat grey_old;
        cv::cvtColor(old(*region), grey_old, cv::COLOR_BGR2GRAY);
        contour_old = estimate_contour(grey_old);

        cv::Mat contour_mat(region->width, region->height, CV_8UC1, cv::Scalar(0));
        for (auto& pt : contour_old)
        {
            contour_mat.data[pt.y * region->width + pt.x] = 0xFF;
        }
        emit debugger_track_pattern(contour_mat);
        state = tracker_state_t::RUNNING;
        sem_post(&state_semaphore);
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
        sem_wait(&state_semaphore);
        state = tracker_state_t::IDLE;
        sem_post(&state_semaphore);
    }
    else
    {
        printf("tracker acq!\n");
        sem_wait(&state_semaphore);
        state = tracker_state_t::ACQUIRING;
        sem_post(&state_semaphore);
    }
    sem_post(&image_semaphore);
}

void Tracker::stop()
{
    sem_wait(&state_semaphore);
    state = tracker_state_t::EXITING;
    sem_post(&state_semaphore);
}

void Tracker::build_hist(cv::Mat grayscale, ulong* hist)
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

ulong Tracker::max_delta(ulong* hist)
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

std::vector<cv::Point> Tracker::estimate_contour(cv::Mat frame)
{

    ulong hist[255];
    build_hist(frame, hist);

    int max_old = max_delta(hist);
    int num_rows = frame.size().height;
    int num_cols = frame.size().width;

    const double SIMILARITY_THRESHOLD = 0.975; //da parametrizzare
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

        if (!contour_candidates_wrt_p1.empty())
        {
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

        if (!contour_candidates_wrt_p1.empty())
        {
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

        if (!contour_candidates_wrt_p1.empty())
        {
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

        if (!contour_candidates_wrt_p1.empty())
        {
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
    }

    return contour;
}

cv::Mat Tracker::shift(cv::Mat &img, int offsetx, int offsety)
{
    cv::Mat trans_mat = (cv::Mat_<double>(2,3) << 1, 0, offsetx, 0, 1, offsety);
    warpAffine(img,img,trans_mat,img.size());
    return img;
}

bool Tracker::equals(cv::Mat a, cv::Mat b, bool* lost_target)
{
    return false;
}

cv::Point estimate_mean_point(std::vector<cv::Point> contour, int* width, int* height)
{
    int y_min=IMAGE_ROWS, x_min=IMAGE_COLS, x_max = 0, y_max = 0;
    for (auto& pt : contour)
    {
        if (pt.x < x_min)
        {
            x_min = pt.x;
        }

        if (pt.x > x_max)
        {
            x_max = pt.x;
        }

        if (pt.y < y_min)
        {
            y_min = pt.y;
        }

        if (pt.y > y_max)
        {
            y_max = pt.y;
        }
    }

    *width = int(fabs(x_max - x_min));
    *height = int(fabs(y_max - y_min));

    return cv::Point((x_max + x_min) / 2, (y_max  + y_min) / 2);
}

void Tracker::run()
{

    while (true)
    {
        sem_wait(&state_semaphore);
        tracker_state_t act_state = state;
        sem_post(&state_semaphore);
        if (tracker_state_t::EXITING == act_state)
        {
            break;
        }
        else if (tracker_state_t::RUNNING == act_state)
        {
            sem_wait(&image_semaphore);
            cv::Mat grey_act;
            cv::cvtColor(act(*region), grey_act, cv::COLOR_BGR2GRAY);
            sem_post(&image_semaphore);

            std::vector<cv::Point> contour_act = estimate_contour(grey_act);
            cv::Mat contour_mat(region->width, region->height, CV_8UC1, cv::Scalar(0));
            for (auto& pt : contour_act)
            {
                contour_mat.data[pt.y * region->width + pt.x] = 0xFF;
            }
            emit debugger_new_frame(contour_mat);

            int old_width, old_height;
            int act_width, act_height;
            cv::Point mean_old = estimate_mean_point(contour_old, &old_width, &old_height);
            cv::Point mean_act = estimate_mean_point(contour_act, &act_width, &act_height);
            int x_mov = (mean_act.x - mean_old.x);
            int y_mov = (mean_act.y - mean_old.y);

            if (old_width != act_width || old_height != act_height)
            {
                printf("LOST TARGET\n");

#if 0
                int new_region_x = original_region->x;
                int new_region_y = original_region->y;
                int new_region_width = original_region->width;
                int new_region_height = original_region->height;

                delete region;
                region = new cv::Rect(new_region_x, new_region_y, new_region_width, new_region_height);
                emit region_updated(*region);
#endif
            }

            else if ((x_mov != 0 || y_mov != 0) && (region->x + x_mov >= 0) && (region->x + x_mov < IMAGE_COLS - region->width)  &&
                     (region->y + y_mov >= 0) && (region->y + y_mov < IMAGE_ROWS - region->height))
            {
                int new_region_x = region->x + x_mov;
                int new_region_y = region->y + y_mov;
                int new_region_width = region->width;
                int new_region_height = region->height;

                delete region;
                region = new cv::Rect(new_region_x, new_region_y, new_region_width, new_region_height);
                emit region_updated(*region);
#if 0
                contour_old.clear();
                for (int i = 0; i < int(contour_act.size()); i++)
                {
                    contour_old[i] = cv::Point(contour_act[i].x, contour_act[i].y);
                }

                cv::Mat contour_mat(region->width, region->height, CV_8UC1, cv::Scalar(0));
                for (auto& pt : contour_old)
                {
                    contour_mat.data[pt.y * region->width + pt.x] = 0xFF;
                }
                emit debugger_track_pattern(contour_mat);
#endif
            }
            else
            {
                //do nothing
            }
        }
    }

    emit thread_quit();
}
