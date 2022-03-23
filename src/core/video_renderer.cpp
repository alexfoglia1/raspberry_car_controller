#include "defs.h"
#include "video_renderer.h"
#include "keymap.h"
#include "widgets.h"
#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <experimental/filesystem>
#include <semaphore.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <QWindow>
#include <QPaintEvent>

GLViewer::GLViewer(QWidget *parent) : QGLWidget(parent)
{
    sem_init(&pixmap_semaphore, 0, 1);
    this->resize(IMAGE_COLS, IMAGE_ROWS);

    QObject::installEventFilter(this);
}

cv::Mat GLViewer::get_frame()
{

    cv::Mat frame(IMAGE_ROWS, IMAGE_COLS, CV_8UC3, cv::Scalar(0, 0, 0));
    if (!pixmap)
    {
        return frame;
    }

    sem_wait(&pixmap_semaphore);
    int matpixelchannelit = 0;
    QImage image = pixmap->toImage();
    sem_post(&pixmap_semaphore);

    for (unsigned int y = 0; y < IMAGE_ROWS; ++y)
    {
        for (unsigned int x = 0; x < IMAGE_COLS; ++x)
        {
                QRgb pxl = image.pixel(x, y);
                uchar* dst = frame.data;
                dst[matpixelchannelit + 0] = qBlue(pxl);
                dst[matpixelchannelit + 1] = qGreen(pxl);
                dst[matpixelchannelit + 2] = qRed(pxl);

                matpixelchannelit += 3;
        }
    }


    return frame;
}

void GLViewer::set_frame(cv::Mat new_frame)
{

    QImage image(new_frame.cols, new_frame.rows, QImage::Format_ARGB32);
    int matpixelchannelit = 0;
    for (int y = 0; y < new_frame.rows; ++y)
    {
        QRgb *destrow = (QRgb*)image.scanLine(y);
        for (int x = 0; x < new_frame.cols; ++x)
        {
            if (new_frame.type() == CV_8UC3)
            {
                int blue = new_frame.data[matpixelchannelit + 0];
                int green = new_frame.data[matpixelchannelit + 1];
                int red = new_frame.data[matpixelchannelit + 2];
                matpixelchannelit += 3;
                destrow[x] = qRgba(red, green, blue, 255);
            }
            else
            {
                int gval = new_frame.data[matpixelchannelit];
                matpixelchannelit++;
                destrow[x] = qRgba(gval, gval, gval, 255);
            }

        }
    }

    sem_wait(&pixmap_semaphore);
    if (pixmap)
    {
        delete pixmap;
    }

    pixmap = new QPixmap(image.width(), image.height());
    pixmap->convertFromImage(image);

    sem_post(&pixmap_semaphore);
    resizeEvent(0);
    update();
}

void GLViewer::clear()
{
    if (pixmap)
    {
        pixmap->fill(Qt::black);
    }
}

bool GLViewer::eventFilter(QObject* target, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
          QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

          emit received_keyboard(keyEvent->key());

    }

    return QGLWidget::eventFilter(target, event);
}

void GLViewer::resizeEvent(QResizeEvent *ev)
{
    Q_UNUSED(ev);

    if (!pixmap)
    {
        return;
    }

    float src_aspect = pixmap->width()/(float)pixmap->height();
    float dest_aspect = width()/(float)height();
    float w;
    if (src_aspect > dest_aspect)
    {
        w = width() - 1;
    }
    else
    {
        w = height()*src_aspect - 1;
    }

    scale = w/pixmap->width();
    scaler = QTransform().scale(scale, scale);
    scalerI = scaler.inverted();
}

void GLViewer::paintEvent(QPaintEvent *ev)
{
    QPainter painter(this);
    if (pixmap)
    {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.setWorldTransform(scaler);
        QRect damaged = scalerI.mapRect(ev->rect());
        painter.drawPixmap(damaged, *pixmap, damaged);
    }
}


VideoRenderer::VideoRenderer()
{
    stopped = false;
    save_frame = false;
    draw_track = false;
    last_duty_cycle = 0.0;
    width = 0;
    height = 0;
    sem_init(&image_semaphore, 0, 1);
}

void VideoRenderer::init_window()
{
    viewer = new GLViewer();

    connect(viewer, SIGNAL(received_keyboard(int)), this, SLOT(on_keyboard(int)));

    std::vector<MenuCvMatWidget::MenuItem> context_menu_items;
    for (auto &algo : image_algorithms)
    {
        context_menu_items.push_back({algo.name, false});
    }

    std::vector<MenuCvMatWidget::MenuItem> system_menu_items =
                                {
                                {"TEGRA DETECTOR", false},
                                {"LSM9DS1 IMU", false},
                                {"VIDEO CAMERA", true},
                                {"ARDUINO NANO", true},
                                {"JOYSTICK", true},
                                {"VOLTAGE (V)", false},
                                {"SYS STATUS", true},

                                };

    context_menu = new MenuCvMatWidget(context_menu_items);
    system_menu = new SystemMenuWidget(system_menu_items, 0, 1, 2, 3, 4, 5, 6);
    system_menu->show();

    speedometer_widget = new SpeedometerWidget();
    speedometer_widget->show();

    target_widget = new TargetWidget();

    attitude_plot = new PlotWidget(100, 360.0, 0.0);
    voltage_plot = new PlotWidget(100, 10.0, 0.0);
    attitude_plot->create_new_serie(cv::String("Yaw"), yellow);
    attitude_plot->create_new_serie(cv::String("Pitch"), red);
    attitude_plot->create_new_serie(cv::String("Roll"), green);

    voltage_plot->create_new_serie(cv::String("Voltage In"), red);
    voltage_plot->create_new_serie(cv::String("Voltage Out"), green);

    next_frame = viewer->get_frame();
    viewer->move(0, 0);
    viewer->show();

    width = next_frame.size().width;
    height = next_frame.size().height;
}

/** Thread job              **/

void VideoRenderer::run()
{
    while (!stopped)
    {
        render_window();
    }

    if (save_frame)
    {
        video->release();
        delete video;

        save_frame = false;
    }

    viewer->deleteLater();
    emit thread_quit();
}

void VideoRenderer::render_window()
{
    /** Copia locale del frame **/
    sem_wait(&image_semaphore);
    cv::Mat cp_next_frame = this->next_frame.clone();
    sem_post(&image_semaphore);

    /** Disegno gli widget abilitati **/
    cv::Size size = next_frame.size();
    context_menu->draw(&cp_next_frame, cv::Point(size.width/30, size.height/30));
    system_menu->draw(&cp_next_frame, cv::Point(size.width/30, 17*size.height/30));
    target_widget->draw(&cp_next_frame);
    speedometer_widget->draw(&cp_next_frame, cv::Point(size.width - 320, size.height - 30), cv::Size(300, 20));
    attitude_plot->draw(&cp_next_frame, cv::Point(10, 10), cv::Size(size.width - 20, size.height - 20));

    /** Passo al viewer il frame con widget **/
    viewer->set_frame(cp_next_frame);

    if (save_frame)
    {
        video->write(cp_next_frame);
    }
    usleep(render_timeout_micros);
}

/*****************************/

/** Remote controlled slots **/

void VideoRenderer::on_video_timeout()
{
    sem_wait(&image_semaphore);
    viewer->clear();
    next_frame = viewer->get_frame();
    sem_post(&image_semaphore);
}

void VideoRenderer::update(attitude_msg attitude)
{
    sem_wait(&image_semaphore);
    printf("yaw(%f) pitch(%f) roll(%f)\n", attitude.yaw, attitude.pitch, attitude.roll);
    attitude_plot->update_serie(0, normalizeAngle(attitude.yaw));
    attitude_plot->update_serie(1, normalizeAngle(attitude.pitch));
    attitude_plot->update_serie(2, normalizeAngle(attitude.roll));
    sem_post(&image_semaphore);
}

void VideoRenderer::update(voltage_msg voltage)
{
    sem_wait(&image_semaphore);
    system_menu->update_voltage(voltage.motor_voltage, last_duty_cycle/255.0);
    sem_post(&image_semaphore);
}

void VideoRenderer::update(actuators_state_msg actuators)
{
    sem_wait(&image_semaphore);
    last_duty_cycle = actuators.throttle_state;
    system_menu->update_system_status(actuators.system_state);
    speedometer_widget->update((char*)&actuators.throttle_state, sizeof(actuators.throttle_state));
    sem_post(&image_semaphore);
}

void VideoRenderer::update(target_msg targets)
{
    sem_wait(&image_semaphore);
    target_widget->update((char*)&targets, sizeof(target_msg));
    sem_post(&image_semaphore);
}

void VideoRenderer::update(quint32 cbit)
{
    sem_wait(&image_semaphore);
    system_menu->update((char*)&cbit, sizeof(quint32));
    sem_post(&image_semaphore);
}

/*****************************/


/** Local controlled slots **/

void VideoRenderer::on_image(cv::Mat frame_from_processor)
{
    sem_wait(&image_semaphore);
    next_frame = frame_from_processor;
    sem_post(&image_semaphore);
}

void VideoRenderer::on_keyboard(int key)
{
    printf("KEY:(%d)\n", key);
    switch (key)
        {
        case TOGGLE_SPEED:
            break;
        case TOGGLE_LOS:
            break;
        case TOGGLE_SS:
            break;
        case VIDEOREC:
            if (!save_frame)
            {
                int tok = 0;
                char buf[256];
                sprintf(buf, "out_%d.avi", tok);
                while(std::experimental::filesystem::exists((const char*)buf))
                {
                    tok += 1;
                    sprintf(buf, "out_%d.avi", tok);
                }

                video = new cv::VideoWriter((const char*)buf, cv::VideoWriter::fourcc('M','J','P','G'), 30, viewer->get_frame().size());
                save_frame = true;

                save_frame = true;
            }
            else
            {
                video->release();
                delete video;

                save_frame = false;
            }
            break;
        case TOGGLE_TGT:
            if (!target_widget->enabled())
            {
                target_widget->show();
            }
            else
            {
                target_widget->hide();
            }
            break;
        case UP_ARROW:
            if (context_menu->enabled())
            {
                navigate_context_menu(-1);
            }
            else
            {
                navigate_system_menu(-1);
            }
            break;

        case DOWN_ARROW:
            if (context_menu->enabled())
            {
                navigate_context_menu(+1);
            }
            else
            {
                navigate_system_menu(+1);
            }
            break;
        case LEFT_ARROW:
            if (context_menu->enabled())
            {
                hide_context_menu();
            }
            break;
        case RIGHT_ARROW:
            if (!context_menu->enabled())
            {
                show_context_menu();
            }
            else
            {
                confirm_context_menu();
            }
            break;
        case ENTER:
            confirm_system_menu();
            break;
        case ESCAPE:
            stopped = true;
            break;
    }
}

/*****************************/


/** Context menu actions   **/

void VideoRenderer::show_context_menu()
{
    context_menu->show();
}
void VideoRenderer::hide_context_menu()
{
    context_menu->hide();
}

void VideoRenderer::navigate_context_menu(int delta)
{
    context_menu->navigateVertical(delta);
}

void VideoRenderer::confirm_context_menu()
{
    if (context_menu->enabled())
    {
        int algorithm_index = context_menu->getSelectedIndex();
        QString algorithm_text = context_menu->getSelectedItem();

        ctx_signal_emitter_t emitter = image_algorithms[algorithm_index].action;
        image_algorithms[algorithm_index].enabled = !image_algorithms[algorithm_index].enabled;
        bool enabled = image_algorithms[algorithm_index].enabled;
        context_menu->setItemText(enabled ? algorithm_text + QString(": ON") : algorithm_text.replace(": ON", ""));
        (this->*emitter)(enabled);
    }
}

/*****************************/


/** System menu actions    **/

void VideoRenderer::navigate_system_menu(int delta)
{
    if (!context_menu->enabled())
    {
        system_menu->navigateVertical(delta);
    }
}

void VideoRenderer::show_system_menu()
{
    system_menu->show();
}

void VideoRenderer::hide_system_menu()
{
    system_menu->hide();
}

void VideoRenderer::confirm_system_menu()
{
    if (!context_menu->enabled())
    {
        if (system_menu->getSelectedIndex() == system_menu->imu_index)
        {
            if (attitude_plot->enabled())
            {
                attitude_plot->hide();
            }
            else
            {
                attitude_plot->show();
            }
        }
    }
}

/*****************************/

/** Signal emitters         **/


void VideoRenderer::clahe_changed_state(bool enabled)
{
    emit signal_clahe_changed_state(enabled);
}

void VideoRenderer::polarity_changed_state(bool enabled)
{
    emit signal_polarity_changed_state(enabled);
}

void VideoRenderer::denoise_changed_state(bool enabled)
{
    emit signal_denoise_changed_state(enabled);
}

void VideoRenderer::r_filter_changed_state(bool enabled)
{
    emit signal_r_filter_changed_state(enabled);
}

void VideoRenderer::g_filter_changed_state(bool enabled)
{
    emit signal_g_filter_changed_state(enabled);
}

void VideoRenderer::b_filter_changed_state(bool enabled)
{
    emit signal_b_filter_changed_state(enabled);
}

/*****************************/

