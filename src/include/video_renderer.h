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

    typedef void (*image_algo_t)(cv::Mat* image);
    typedef struct
    {
        QString name;
        bool active;
        image_algo_t algorithm;
    } image_algo_state_t;

public:
    static const int RENDER_FREQ_HZ = 30;
    double render_timeout_micros = 1e6/RENDER_FREQ_HZ;

    VideoRenderer();
    void init_window();

protected:
    void run() override;

public slots:
    /** Remote controlled slots **/
    void clear();
    void update(image_msg image);
    void update(voltage_msg voltage);
    void update(attitude_msg attitude);
    void update(actuators_state_msg actuators);
    void update(target_msg targets);
    void update(quint32 cbit);

    /** Local controlled slots **/
    void on_keyboard(int key);
    void toggle_widget(int widget);
    void toggle_videorec(int fps);

    /** Context menu actions **/
    void show_context_menu();
    void hide_context_menu();
    void navigate_context_menu(int delta);
    void confirm_context_menu();

signals:
    void thread_quit();

private:
    sem_t image_semaphore;
    GLViewer* viewer;
    cv::Mat next_frame;
    cv::VideoWriter* video;
    bool save_frame;
    bool stopped;

    /** Widgets **/
    MenuCvMatWidget* context_menu;

    /** Algorithms **/
    std::vector<image_algo_state_t> image_algorithms =
    {
     {"CLAHE",    false, &clahe},
     {"POLARITY", false, &polarity},
     {"R-FILTER", false, [](cv::Mat* image){channel_filter(image, 2);}},
     {"G-FILTER", false, [](cv::Mat* image){channel_filter(image, 1);}},
     {"B-FILTER", false, [](cv::Mat* image){channel_filter(image, 0);}}
    };

    void render_window();
    void start_videorec(int fps);

};

#endif //VIDEO_RENDERER_H
