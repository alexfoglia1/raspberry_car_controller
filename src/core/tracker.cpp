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

        emit region_updated(this->region);
        emit valid_acquiring_area(false);
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

bool Tracker::acquire_reference_frame()
{
    double candidate_timestamp;
    cv::Mat reference_frame_candidate = extract_roi(&candidate_timestamp);

    bool trackable_image = true;
    if (trackable_image)
    {
        this->reference_image_time_s = candidate_timestamp;
        this->reference_image = reference_frame_candidate;
        this->reset_flag = true;
    }

    return trackable_image;
}

void Tracker::track()
{
    double timestamp_s;
    cv::Mat act_image = extract_roi(&timestamp_s);

    printf("delta_t (%f)\n", timestamp_s - reference_image_time_s);
}

void Tracker::coast()
{

}
