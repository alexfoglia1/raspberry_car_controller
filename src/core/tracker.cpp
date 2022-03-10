#include "tracker.h"

#include <math.h>


Tracker::Tracker(cv::Rect region)
{
    this->region.x = region.x;
    this->region.y = region.y;
    this->region.width = region.width;
    this->region.height = region.height;
    this->original_region.x = region.x;
    this->original_region.y = region.y;
    this->original_region.width = region.width;
    this->original_region.height = region.height;

    this->reset_flag = false;

    this->state = tracker_state_t::IDLE;
    this->last_camera_frame = cv::Mat(IMAGE_COLS, IMAGE_ROWS, CV_8UC3, cv::Scalar(0, 0 ,0));
    this->reference_image = cv::Mat(region.width, region.height, CV_8UC1, cv::Scalar(0, 0, 0));
    this->reference_image_time_s = -1.0;
    sem_init(&frame_semaphore, 0, 1);
    sem_init(&state_semaphore, 0, 1);
}

void Tracker::on_camera_image(cv::Mat frame_from_camera)
{
    safe_update_frame(frame_from_camera);
}

void Tracker::on_update_state(tracker_state_t new_state)
{
    safe_update_state(new_state);
}

void Tracker::run()
{
    bool stopped = false;
    while (!stopped)
    {
        tracker_state_t act_state = safe_get_state();

        switch (act_state)
        {
            case tracker_state_t::IDLE:
                reset_tracker();
                break;
            case tracker_state_t::ACQUIRING:
                emit valid_acquiring_area(acquire_reference_frame());
                break;
            case tracker_state_t::RUNNING:
                emit tracker_running();
                track();
                break;
            case tracker_state_t::COASTING:
                coast();
                break;
            case tracker_state_t::EXITING:
                stopped = true;
                break;
        }

    }

    emit thread_quit();
}

void Tracker::reset_tracker()
{
    if (this->reset_flag)
    {
        this->region.x = this->original_region.x;
        this->region.y = this->original_region.y;
        this->region.width = this->original_region.width;
        this->region.height = this->original_region.height;
        this->last_camera_frame = cv::Mat(IMAGE_COLS, IMAGE_ROWS, CV_8UC3, cv::Scalar(0, 0 ,0));
        this->reference_image = cv::Mat(region.width, region.height, CV_8UC1, cv::Scalar(0, 0, 0));
        this->reference_image_time_s = -1.0;
        this->reset_flag = false;
        this->reference_bounds.clear();

        emit region_updated(this->region);
        emit valid_acquiring_area(false);
        emit tracker_idle();
    }
}

void Tracker::safe_update_frame(cv::Mat frame)
{
    sem_wait(&frame_semaphore);
    last_camera_frame = frame;
    sem_post(&frame_semaphore);
}

void Tracker::safe_update_state(tracker_state_t new_state)
{
    sem_wait(&state_semaphore);

    bool valid = is_valid_state_transition(state, new_state);
    if (valid)
    {
        state = new_state;
    }
    emit valid_state_transition(valid);

    sem_post(&state_semaphore);
}

cv::Mat Tracker::safe_get_frame()
{
    sem_wait(&frame_semaphore);
    cv::Mat last_frame = last_camera_frame;
    sem_post(&frame_semaphore);

    return last_frame;
}

tracker_state_t Tracker::safe_get_state()
{
    sem_wait(&state_semaphore);
    tracker_state_t last_state = state;
    sem_post(&state_semaphore);

    return last_state;
}

bool Tracker::is_valid_state_transition(tracker_state_t from, tracker_state_t to)
{
    for (auto& pair : valid_transitions)
    {
        if (pair.first == from && pair.second == to)
        {
            return true;
        }
    }

    return false;
}

cv::Mat Tracker::extract_roi(double* timestamp_s)
{
    cv::Mat act_frame_bgr = safe_get_frame();
    *timestamp_s = micros_since_epoch() * 1e-6;

    cv::Mat roi(region.width, region.height, CV_8UC1, cv::Scalar(0));
    cv::cvtColor(act_frame_bgr(region), roi, cv::COLOR_BGR2GRAY);

    return roi;
}

std::vector<cv::Point> Tracker::estimate_target_bounds(cv::Mat frame)
{
    uchar hist[256];
    for (int i = 0; i < 256; i++)
    {
        hist[i] = 0;
    }

    for (int i = 0; i < frame.dataend - frame.data; i++)
    {
        hist[frame.data[i]] += 1;
    }

    double mean = 0.0;
    for (int i = 0; i < 256; i++)
    {
        mean += (double)(hist[i]);
    }
    mean /= 256.0;

    double sd = 0.0;
    for (int i = 0; i < 256; i++)
    {
        sd += pow((double)(hist[i]) - mean, 2);
    }
    sd = sqrt(sd);

    cv::Mat target;
    cv::threshold(frame, target, mean + sd, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(target, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);

    std::vector<cv::Point> bounds;
    if (!contours.empty())
    {
        for (size_t idx = 0; idx < contours.size(); idx++)
        {
            for (auto& pt : contours[idx])
            {
                bounds.push_back(cv::Point(pt.x, pt.y));
            }
        }
    }

    return bounds;
}

bool Tracker::acquire_reference_frame()
{
    double candidate_timestamp;
    cv::Mat reference_frame_candidate = extract_roi(&candidate_timestamp);

    std::vector<cv::Point> bounds = estimate_target_bounds(reference_frame_candidate);

    if (!bounds.empty())
    {
        this->reference_image_time_s = candidate_timestamp;
        this->reference_image = reference_frame_candidate;
        for (auto& pt : bounds)
        {
            this->reference_bounds.push_back(pt);
        }
        this->reset_flag = true;
    }

    return !bounds.empty();

}

std::vector<cv::Point> extreme_points(std::vector<cv::Point> pts)
{
    cv::Point extLeft  = *std::min_element(pts.begin(), pts.end(),
                          [](const cv::Point& lhs, const cv::Point& rhs) {
                              return lhs.x < rhs.x;
                      });
    cv::Point extRight = *std::max_element(pts.begin(), pts.end(),
                          [](const cv::Point& lhs, const cv::Point& rhs) {
                              return lhs.x < rhs.x;
                      });
    cv::Point extTop   = *std::min_element(pts.begin(), pts.end(),
                          [](const cv::Point& lhs, const cv::Point& rhs) {
                              return lhs.y < rhs.y;
                      });
    cv::Point extBot   = *std::max_element(pts.begin(), pts.end(),
                          [](const cv::Point& lhs, const cv::Point& rhs) {
                              return lhs.y < rhs.y;
                      });

    return std::vector<cv::Point>{extLeft, extRight, extTop, extBot};
}

void Tracker::track()
{
    double timestamp_s;
    cv::Mat act_image = extract_roi(&timestamp_s);

    std::vector<cv::Point> act_bounds = estimate_target_bounds(act_image);

    if (act_bounds.empty())
    {

    }
    else
    {
        std::vector<cv::Point> reference_extreme_points = extreme_points(reference_bounds);
        std::vector<cv::Point> actual_extreme_points = extreme_points(act_bounds);

        double extLeftDx = actual_extreme_points[0].x - reference_extreme_points[0].x;
        double extRightDx = actual_extreme_points[1].x - reference_extreme_points[1].x;
        double extTopDx = actual_extreme_points[2].x - reference_extreme_points[2].x;
        double extBotDx = actual_extreme_points[3].x - reference_extreme_points[3].x;

        double extLeftDy = actual_extreme_points[0].y - reference_extreme_points[0].y;
        double extRightDy = actual_extreme_points[1].y - reference_extreme_points[1].y;
        double extTopDy = actual_extreme_points[2].y - reference_extreme_points[2].y;
        double extBotDy = actual_extreme_points[3].y - reference_extreme_points[3].y;

        double mean_mov_x = (extLeftDx + extRightDx + extTopDx + extBotDx)/4.0;
        double sd_mov_x =  sqrt((pow(extLeftDx - mean_mov_x, 2) + pow(extRightDx - mean_mov_x, 2) +
                                 pow(extTopDx - mean_mov_x, 2) + pow(extTopDx - mean_mov_x, 2))/4.0);
        double mean_mov_y = (extLeftDy + extRightDy + extTopDy + extBotDy)/4.0;
        double sd_mov_y =  sqrt((pow(extLeftDy - mean_mov_y, 2) + pow(extRightDy - mean_mov_y, 2) +
                                 pow(extTopDy - mean_mov_y,  2) + pow(extTopDy - mean_mov_y, 2))/4.0);

        printf("mean_mov_x(%f), mean_mov_y(%f)\n", mean_mov_x, mean_mov_y);
        printf("sd_mov_x(%f), sd_mov_y(%f)\n", sd_mov_x, sd_mov_y);

        const double MOVEMENT_THRESHOLD = 25;
        if (sd_mov_y < MOVEMENT_THRESHOLD && sd_mov_x < MOVEMENT_THRESHOLD)
        {
            this->region.x += mean_mov_x;
            this->region.y += mean_mov_y;

            emit region_updated(region);
        }
    }

}

void Tracker::coast()
{

}
