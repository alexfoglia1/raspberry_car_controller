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
    this->est_speed_x = 0.0;
    this->est_speed_y = 0.0;
    this->coasting_t0_s = 0.0;
    this->coasting_attempts = 0;
    this->ref_area = 0.0;
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
                idle();
                break;
            case tracker_state_t::ACQUIRING:
                acquire();
                break;
            case tracker_state_t::RUNNING:
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
            switch(to)
            {
            case tracker_state_t::RUNNING:
                return !reference_bounds.empty();
            default:
                return true;
            }
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

    double sum = 0.0;
    for (int i = 0; i < 256; i++)
    {
        sum += hist[i];
    }

    double mean = 0.0;
    for (int i = 0; i < 256; i++)
    {
        mean += (i * (double)(hist[i]));
    }
    mean /= sum;

    mean = int(mean);

    cv::Mat target;
    cv::threshold(frame, target, mean, 255, cv::THRESH_BINARY);

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
    this->coasting_attempts = 0;

    double candidate_timestamp;
    cv::Mat reference_frame_candidate = extract_roi(&candidate_timestamp);

    std::vector<cv::Point> bounds = estimate_target_bounds(reference_frame_candidate);

    if (!bounds.empty())
    {
        std::vector<cv::Point> ext_pts = extreme_points(bounds);
        double base = ext_pts[1].x - ext_pts[0].x;
        double height = ext_pts[3].y - ext_pts[1].y;
        ref_area = base * height;

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

std::vector<cv::Point> Tracker::extreme_points(std::vector<cv::Point> pts)
{
    double min_x = region.width;
    double max_x = 0;
    double min_y = region.height;
    double max_y = 0;
    for (auto& pt : pts)
    {
        if (pt.x < min_x)
        {
            min_x = pt.x;
        }

        if (pt.x > max_x)
        {
            max_x = pt.x;
        }

        if (pt.y < min_y)
        {
            min_y = pt.y;
        }

        if (pt.y > max_y)
        {
            max_y = pt.y;
        }
    }


    std::vector<cv::Point> leftmost;
    std::vector<cv::Point> rightmost;
    std::vector<cv::Point> upmost;
    std::vector<cv::Point> downmost;

    for (auto& pt : pts)
    {
        if (pt.x == min_x)
        {
            leftmost.push_back(pt);
        }

        if (pt.x == max_x)
        {
            rightmost.push_back(pt);
        }

        if (pt.y == min_y)
        {
            upmost.push_back(pt);
        }

        if (pt.y == max_y)
        {
            downmost.push_back(pt);
        }
    }

    cv::Point* extLeft;
    cv::Point* extRight;
    cv::Point* extTop;
    cv::Point* extBot;

    int min_y_leftmost = region.height;
    int max_x_upmost = 0;
    int max_y_rightmost = 0;
    int min_x_downmost = region.width;

    for (auto& pt : leftmost)
    {
        /** Nei leftmost voglio quello più in alto **/
        if (pt.y < min_y_leftmost)
        {
            min_y_leftmost = pt.y;
            extLeft = &pt;
        }
    }

    for (auto& pt : upmost)
    {
        /** Negli upmost voglio quello più a destra **/
        if (pt.x > max_x_upmost)
        {
            max_x_upmost = pt.x;
            extRight = &pt;
        }
    }

    for (auto& pt : rightmost)
    {
        /** Nei rightmost voglio quello più in basso **/
        if (pt.y > max_y_rightmost)
        {
            max_y_rightmost = pt.y;
            extTop = &pt;
        }
    }

    for (auto& pt : downmost)
    {
        /** Nei downmost voglio quello più a sinistra **/
        if (pt.x < min_x_downmost)
        {
            min_x_downmost = pt.x;
            extBot = &pt;
        }
    }

    emit extreme_points_updated(*extLeft, *extRight, *extTop, *extBot);
    return std::vector<cv::Point>{*extLeft, *extRight, *extTop, *extBot};
}

bool Tracker::bounds_matches(std::vector<cv::Point> act_bounds, double* mean_mov_x, double* mean_mov_y, double* sd_x, double* sd_y)
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

    *mean_mov_x = (extLeftDx + extRightDx + extTopDx + extBotDx)/4.0;
    double sd_mov_x =  sqrt((pow(extLeftDx - *mean_mov_x, 2) + pow(extRightDx - *mean_mov_x, 2) +
                             pow(extTopDx - *mean_mov_x, 2) + pow(extTopDx - *mean_mov_x, 2))/4.0);
    *mean_mov_y = (extLeftDy + extRightDy + extTopDy + extBotDy)/4.0;
    double sd_mov_y =  sqrt((pow(extLeftDy - *mean_mov_y, 2) + pow(extRightDy - *mean_mov_y, 2) +
                             pow(extTopDy - *mean_mov_y,  2) + pow(extTopDy - *mean_mov_y, 2))/4.0);

    *sd_x = sd_mov_x;
    *sd_y = sd_mov_y;


    double MOVEMENT_THRESHOLD = sqrt(ref_area);

    return sd_mov_x <= MOVEMENT_THRESHOLD / 4 || sd_mov_y <= MOVEMENT_THRESHOLD / 4;
}


void Tracker::idle()
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
        this->est_speed_x = 0.0;
        this->est_speed_y = 0.0;
        this->coasting_t0_s = 0.0;
        this->coasting_attempts = 0;
        this->ref_area = 0.0;
        this->reset_flag = false;
        this->reference_bounds.clear();

        emit region_updated(this->region);
        emit valid_acquiring_area(false);
        emit tracker_idle();
    }
}

void Tracker::acquire()
{
    est_speed_x = 0.0;
    est_speed_y = 0.0;

    bool acquired = acquire_reference_frame();
    emit valid_acquiring_area(acquired);
}


void Tracker::track()
{
    emit tracker_running();

    double timestamp_s;
    cv::Mat act_image = extract_roi(&timestamp_s);
    std::vector<cv::Point> act_bounds = estimate_target_bounds(act_image);

    bool tgt_lost = true;

    if (!act_bounds.empty())
    {
        double mean_mov_x;
        double mean_mov_y;
        double sd_x;
        double sd_y;
        if (bounds_matches(act_bounds, &mean_mov_x, &mean_mov_y, &sd_x, &sd_y))
        {
            this->region.x += mean_mov_x;
            this->region.y += mean_mov_y;

            double delta_t_s = (timestamp_s - reference_image_time_s);

            this->est_speed_x = (mean_mov_x + sd_x) / delta_t_s;
            this->est_speed_y = (mean_mov_y + sd_y) / delta_t_s;
            tgt_lost = false;
            emit region_updated(region);
        }
    }

    if (tgt_lost)
    {
        this->coasting_attempts = 0;
        this->coasting_t0_s = micros_since_epoch() * 1e-6;
        safe_update_state(tracker_state_t::COASTING);
    }
}

void Tracker::coast()
{

    emit tracker_coasting();
    printf("[%d] coasting(%f, %f))\n", coasting_attempts, est_speed_x, est_speed_y);

    double t_s;
    cv::Mat act_image = extract_roi(&t_s);

    double dt =  t_s - this->coasting_t0_s;

    double x_mov = dt * est_speed_x;
    double y_mov = dt * est_speed_y;

    const int MAX_COASTING_ATTEMPTS = 20;
    if (this->region.x + x_mov > IMAGE_COLS ||
        this->region.x + x_mov < 0 ||
        this->region.y + y_mov > IMAGE_ROWS ||
        this->region.y + y_mov < 0 ||
        coasting_attempts > MAX_COASTING_ATTEMPTS)
    {
        safe_update_state(tracker_state_t::ACQUIRING);
    }
    else
    {
        double mean_mov_x;
        double mean_mov_y;
        double sd_x;
        double sd_y;

        std::vector<cv::Point> act_bounds = estimate_target_bounds(act_image);
        if (!act_bounds.empty() && bounds_matches(act_bounds, &mean_mov_x, &mean_mov_y, &sd_x, &sd_y))
        {
            this->region.x += mean_mov_x;
            this->region.y += mean_mov_y;

            this->est_speed_x = (mean_mov_x + sd_x) / dt;
            this->est_speed_y = (mean_mov_y + sd_y) / dt;

            safe_update_state(tracker_state_t::RUNNING);
        }
        else
        {
            this->region.x += x_mov;
            this->region.y += y_mov;

            /** rimango in coasting **/
        }

        emit region_updated(region);
        coasting_attempts += 1;
    }
}
