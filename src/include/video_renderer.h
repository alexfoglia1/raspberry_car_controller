#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include "widgets.h"
#include "defs.h"
#include "video_algo.h"

#include <semaphore.h>
#include <opencv2/opencv.hpp>
#include <QThread>
#include <QtOpenGL/QGLWidget>
#include <QObject>

class GLViewer : public QGLWidget
{
    Q_OBJECT
public:
    GLViewer(QWidget *parent = NULL);
    void set_frame(cv::Mat newframe);
    void clear();
    cv::Mat get_frame();

    void resizeEvent(QResizeEvent* ev);
    virtual void paintEvent(QPaintEvent* ev);

signals:
    void received_keyboard(int key);

protected:
    virtual bool eventFilter(QObject *target, QEvent *event);

    qreal scale;
    QTransform scaler, scalerI;
    QPixmap* pixmap = 0;
};

class VideoRenderer : public QThread
{
    Q_OBJECT

public:
    typedef void (VideoRenderer::*ctx_signal_emitter_t)(bool);
    typedef struct
    {
        QString name;
        bool enabled;
        ctx_signal_emitter_t action;
    } image_algorithm_state_t;

    static const int RENDER_FREQ_HZ = 30;
    double render_timeout_micros = 1e6/RENDER_FREQ_HZ;
    int width;
    int height;

    VideoRenderer();
    void init_window();

protected:
    /** Thread job **/
    void run() override;
    void render_window();

public slots:
    /** Remote controlled slots **/
    void on_video_timeout();
    void update(voltage_msg voltage);
    void update(attitude_msg attitude);
    void update(actuators_state_msg actuators);
    void update(target_msg targets);
    void update(quint32 cbit);

    /** Local controlled slots **/
    void on_image(cv::Mat image);
    void on_tracker_image(cv::Mat image);
    void on_tracker_update(cv::Rect);
    void on_keyboard(int key);

    /** Context menu actions **/
    void show_context_menu();
    void hide_context_menu();
    void navigate_context_menu(int delta);
    void confirm_context_menu();

    /** System menu actions **/
    void show_system_menu();
    void hide_system_menu();
    void navigate_system_menu(int delta);
    void confirm_system_menu();

signals:
    /** signals to clients **/
    void signal_clahe_changed_state(bool enabled);
    void signal_polarity_changed_state(bool enabled);
    void signal_denoise_changed_state(bool enabled);
    void signal_r_filter_changed_state(bool enabled);
    void signal_g_filter_changed_state(bool enabled);
    void signal_b_filter_changed_state(bool enabled);
    void thread_quit();

private:
    /** signal emitters **/
    void clahe_changed_state(bool enabled);
    void polarity_changed_state(bool enabled);
    void denoise_changed_state(bool enabled);
    void r_filter_changed_state(bool enabled);
    void g_filter_changed_state(bool enabled);
    void b_filter_changed_state(bool enabled);

    sem_t image_semaphore;
    sem_t track_semaphore;
    GLViewer* viewer;
    GLViewer* tracker_debugger;
    cv::Mat next_frame;
    cv::VideoWriter* video;
    bool save_frame;
    bool stopped;
    double last_duty_cycle;

    /** Widgets **/
    MenuCvMatWidget* context_menu;
    SystemMenuWidget* system_menu;
    SpeedometerWidget* speedometer_widget;
    TargetWidget* target_widget;

    /** Tracker region **/
    cv::Rect tracker_region;

    std::vector<image_algorithm_state_t> image_algorithms =
    {
        {"CLAHE",       false,  &VideoRenderer::clahe_changed_state},
        {"POLARITY",    false,  &VideoRenderer::polarity_changed_state},
        {"DENOISE",     false,  &VideoRenderer::denoise_changed_state},
        {"R-FILTER",    false,  &VideoRenderer::r_filter_changed_state},
        {"G-FILTER",    false,  &VideoRenderer::g_filter_changed_state},
        {"B-FILTER",    false,  &VideoRenderer::b_filter_changed_state}
    };


};

#endif //VIDEO_RENDERER_H
