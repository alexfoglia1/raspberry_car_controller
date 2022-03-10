#ifndef TRACKER_H
#define TRACKER_H
#include <opencv2/opencv.hpp>
#include <QThread>
#include <semaphore.h>
#include <vector>

#include "utils.h"
#include "defs.h"

class Tracker : public QThread
{
    Q_OBJECT


public:
    Tracker(cv::Rect region);

protected:
    void run();

public slots:
    void on_camera_image(cv::Mat frame_from_camera);
    void on_update_state(tracker_state_t new_state);

signals:
    void debugger_new_frame(cv::Mat frame);
    void debugger_track_pattern(cv::Mat frame);
    void valid_acquiring_area(bool valid);
    void valid_state_transition(bool valid);
    void region_updated(cv::Rect new_region);
    void thread_quit();

private:
    std::vector<std::pair<tracker_state_t, tracker_state_t>> valid_transitions =
    {
        /* actual state */              /* next state */
        {tracker_state_t::IDLE,         tracker_state_t::IDLE},
        {tracker_state_t::IDLE,         tracker_state_t::ACQUIRING},
        {tracker_state_t::IDLE,         tracker_state_t::EXITING},

        {tracker_state_t::ACQUIRING,    tracker_state_t::IDLE},
        {tracker_state_t::ACQUIRING,    tracker_state_t::ACQUIRING},
        {tracker_state_t::ACQUIRING,    tracker_state_t::RUNNING},
        {tracker_state_t::ACQUIRING,    tracker_state_t::EXITING},

        {tracker_state_t::RUNNING,      tracker_state_t::COASTING},
        {tracker_state_t::RUNNING,      tracker_state_t::RUNNING},
        {tracker_state_t::RUNNING,      tracker_state_t::IDLE},
        {tracker_state_t::RUNNING,      tracker_state_t::EXITING},

        {tracker_state_t::COASTING,     tracker_state_t::COASTING},
        {tracker_state_t::COASTING,     tracker_state_t::RUNNING},
        {tracker_state_t::IDLE,         tracker_state_t::IDLE},
        {tracker_state_t::COASTING,     tracker_state_t::EXITING}
    };

    tracker_state_t state;
    cv::Mat last_camera_frame;
    cv::Mat reference_image;
    double reference_image_time_s;
    cv::Rect region;
    cv::Rect original_region;
    bool reset_flag;
    sem_t frame_semaphore;
    sem_t state_semaphore;

    /** Internal tracker state methods **/
    void reset_tracker();
    void safe_update_frame(cv::Mat frame);
    void safe_update_state(tracker_state_t new_state);
    cv::Mat safe_get_frame();
    tracker_state_t safe_get_state();
    bool is_valid_state_transition(tracker_state_t from, tracker_state_t to);

    /** Elaboration methods **/
    cv::Mat extract_roi(double* timestamp_s);
    bool acquire_reference_frame();
    void track();
    void coast();

};

#endif // TRACKER_H
