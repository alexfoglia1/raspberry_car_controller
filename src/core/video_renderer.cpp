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

    int matpixelchannelit = 0;
    QImage image = pixmap->toImage();
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
            int blue = new_frame.data[matpixelchannelit + 0];
            int green = new_frame.data[matpixelchannelit + 1];
            int red = new_frame.data[matpixelchannelit + 2];
            matpixelchannelit += 3;

            destrow[x] = qRgba(red, green, blue, 255);
        }
    }

    if (pixmap)
    {
        delete pixmap;
    }

    pixmap = new QPixmap(image.width(), image.height());
    pixmap->convertFromImage(image);
    resizeEvent(0);
    update();
}

void GLViewer::clear()
{
    if (pixmap)
    {
        delete pixmap;
    }
    pixmap = nullptr;
}

bool GLViewer::eventFilter(QObject* target, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
          QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
          printf("%d\n", keyEvent->key());

          emit received_keyboard(keyEvent->key());

    }

    return QGLWidget::eventFilter(target, event);
}

void GLViewer::resizeEvent(QResizeEvent *ev)
{
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

    sem_init(&image_semaphore, 0, 1);
}

void VideoRenderer::init_window()
{
    viewer = new GLViewer();
    connect(viewer, SIGNAL(received_keyboard(int)), this, SLOT(on_keyboard(int)));

    widgets::throttlestate::init();
    widgets::los::init();
    widgets::targets::init();
    widgets::systemstatus::init();
    //widgets::devmode::init();

    next_frame = viewer->get_frame();
    viewer->move(0, 0);
    viewer->show();
}

void VideoRenderer::update(image_msg image)
{
    std::vector<char> data(image.data, image.data + image.len);

    sem_wait(&image_semaphore);
    cv::Mat frame_from_camera = cv::imdecode(cv::Mat(data), 1);
    next_frame = frame_from_camera;
    sem_post(&image_semaphore);
}

void VideoRenderer::clear()
{
    sem_wait(&image_semaphore);
    viewer->clear();
    next_frame = viewer->get_frame();
    sem_post(&image_semaphore);
}

void VideoRenderer::update(attitude_msg attitude)
{
    widgets::los::update(attitude.pitch, attitude.roll, attitude.yaw);
}

void VideoRenderer::update(voltage_msg voltage)
{
    widgets::systemstatus::updateMotorVoltageIn(voltage.motor_voltage);
    widgets::throttlestate::updateVoltageIn(voltage.motor_voltage);
}

void VideoRenderer::update(actuators_state_msg actuators)
{
    widgets::systemstatus::updateRemoteSystemState(actuators.system_state);
    widgets::systemstatus::updateMotorVoltageOut(actuators.throttle_state);
    widgets::throttlestate::updateThrottle(actuators.throttle_state);
}

void VideoRenderer::update(target_msg targets)
{
    widgets::targets::update(targets);
}

void VideoRenderer::update(quint32 cbit)
{
    widgets::systemstatus::updateCbit(cbit);
}

void VideoRenderer::on_keyboard(int key)
{
    switch (key)
        {
        case TOGGLE_SPEED:
            toggle_widget(widgets::THROTTLESTATE);
            break;
        case TOGGLE_LOS:
            toggle_widget(widgets::LOS);
            break;
        case TOGGLE_MENU:
            toggle_widget(widgets::HELP);
            break;
        case TOGGLE_SS:
            toggle_widget(widgets::SYSTEM_STATUS);
            break;
        case TOGGLE_JS:
            toggle_widget(widgets::JS_HELP);
            break;
        case VIDEOREC:
            toggle_videorec(30);
            break;
        case TOGGLE_TGT:
            toggle_widget(widgets::TARGETS);
            break;
        case DEV_MODE:
            //controller->postEvent(Controller::controller_event_t::received_toggle_devmode);
            break;
        case UP_ARROW:
            //handle up arrow
            break;
        case DOWN_ARROW:
            //handle down arrow
            break;
        case LEFT_ARROW:
            //handle left arrow
            break;
        case RIGHT_ARROW:
            //handle right arrow
            break;
        case ESCAPE:
            stopped = true;
            break;
    }
}


void VideoRenderer::toggle_widget(int widget)
{
    widgets::is_enabled[widget] = !widgets::is_enabled[widget];
}

void VideoRenderer::toggle_videorec(int fps)
{
    if (!save_frame)
    {
        start_videorec(fps);

        save_frame = true;
    }
    else
    {
        video->release();
        delete video;

        save_frame = false;
    }

    widgets::is_enabled[widgets::VIDEO_REC] = save_frame;
}

void VideoRenderer::show_context_menu()
{
    context_menu.show();
}
void VideoRenderer::hide_context_menu()
{
    context_menu.hide();
}
void VideoRenderer::navigate_context_menu(int delta)
{
    context_menu.navigate(delta);
}


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

    emit thread_quit();
}

void VideoRenderer::render_window()
{
    sem_wait(&image_semaphore);
    cv::Size size = next_frame.size();
    widgets::los::draw(&next_frame, 10U + widgets::los::LOS_RAY, size.height - widgets::los::LOS_RAY -  10U);
    widgets::throttlestate::draw(&next_frame, size.width - 320U, size.height - 30U);
    widgets::targets::draw(&next_frame);
    widgets::help::draw(&next_frame, size.width/2, size.height/2);
    widgets::js_help::draw(&next_frame, size.width/2, size.height/2);
    widgets::videorec::draw(&next_frame, size.width - 40U, 50U);
    widgets::systemstatus::draw(&next_frame, size.width - 130U, size.height/2);

    context_menu.draw(&next_frame);

    //widgets::devmode::draw(imagewindow, size.width/2, size.height/2);


    viewer->set_frame(next_frame);

    if (save_frame)
    {
        video->write(next_frame);
    }

    sem_post(&image_semaphore);
    usleep(render_timeout_micros);
}

void VideoRenderer::start_videorec(int fps)
{
    int tok = 0;
    char buf[256];
    sprintf(buf, "out_%d.avi", tok);
    while(std::experimental::filesystem::exists((const char*)buf))
    {
        tok += 1;
        sprintf(buf, "out_%d.avi", tok);
    }

    video = new cv::VideoWriter((const char*)buf, cv::VideoWriter::fourcc('M','J','P','G'), fps, viewer->get_frame().size());
    save_frame = true;
}
