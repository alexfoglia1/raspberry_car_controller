#include <QApplication>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <easy/profiler.h>

#include "defs.h"
#include "cbit.h"
#include "data_interface.h"
#include "video_interface.h"
#include "video_renderer.h"
#include "video_algo.h"
#include "joystick.h"
#include "tracker.h"

const char* DEFAULT_RASPBY_ADDR = "192.168.1.25";

int main(int argc, char** argv)
{
    profiler::startListen();
    printf("%s v%s.%s\n", APP_NAME, MAJOR_VERS, MINOR_VERS);
    if (argc == 1)
    {
        printf("USAGE: %s [raspberry_address]\n", argv[0]);
        printf("Trying to use %s\n\n", DEFAULT_RASPBY_ADDR);
    }

    /** Init Qt Application **/
    QApplication app(argc, argv);
    app.setApplicationDisplayName(QString("%1 %2.%3").arg(APP_NAME).arg(MAJOR_VERS).arg(MINOR_VERS));
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(QString("%1.%2").arg(MAJOR_VERS).arg(MINOR_VERS));

    /** Register meta types **/
    qRegisterMetaType<joystick_msg>();
    qRegisterMetaType<attitude_msg>();
    qRegisterMetaType<voltage_msg>();
    qRegisterMetaType<actuators_state_msg>();
    qRegisterMetaType<target_msg>();
    qRegisterMetaType<image_msg>();
    qRegisterMetaType<comp_t>();
    qRegisterMetaType<cv::Mat>();
    qRegisterMetaType<cv::Rect>();

    /** Create cbit instance **/
    Cbit* cbit = new Cbit();

    /** Create window render **/
    VideoRenderer* renderer = new VideoRenderer();

    /** Start renerer **/
    renderer->init_window();
    renderer->start();

    /** Create tracker instance **/
    cv::Rect tracker_region(renderer->width / 2 - 100, renderer->height / 2 - 100, 200, 200);
    Tracker* tracker = new Tracker(tracker_region);
    renderer->on_tracker_update(tracker_region);
    QObject::connect(tracker, SIGNAL(debugger_frame(cv::Mat)), renderer, SLOT(on_tracker_image(cv::Mat)));
    QObject::connect(tracker, SIGNAL(region_updated(cv::Rect)), renderer, SLOT(on_tracker_update(cv::Rect)));

    /** Create data interface **/
    DataInterface* iface = new DataInterface(argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1], 3000);
    QObject::connect(iface, SIGNAL(received_attitude(attitude_msg)), renderer, SLOT(update(attitude_msg)));
    QObject::connect(iface, SIGNAL(received_voltage(voltage_msg)), renderer, SLOT(update(voltage_msg)));
    QObject::connect(iface, SIGNAL(received_actuators(actuators_state_msg)), renderer, SLOT(update(actuators_state_msg)));
    QObject::connect(iface, SIGNAL(received_targets(target_msg)), renderer, SLOT(update(target_msg)));

    /** Connecting data to cbit **/
    QObject::connect(iface, &DataInterface::data_timeout, cbit, [cbit](comp_t component)
    {
    switch (component)
    {
    case comp_t::ARDUINO:
        cbit->update_cbit(true, ARDUINO_NODATA);
        break;
    case comp_t::ATTITUDE:
        cbit->update_cbit(true, ATTITUDE_NODATA);
        break;
    case comp_t::MOTORS:
        cbit->update_cbit(true, MOTORS_NODATA);
        break;
    case comp_t::TEGRA:
        cbit->update_cbit(true, TEGRA_NODATA);
    case comp_t::VIDEO:
        /** Handled by VideoInterface **/
        break;
    case comp_t::JOYSTICK:
        /** Handled by JoystickInput **/
        break;
    }
    });
    QObject::connect(iface, &DataInterface::received_attitude, cbit, [cbit](){cbit->update_cbit(false, ATTITUDE_NODATA);});
    QObject::connect(iface, &DataInterface::received_voltage, cbit, [cbit](){cbit->update_cbit(false, ARDUINO_NODATA);});
    QObject::connect(iface, &DataInterface::received_actuators, cbit, [cbit](){cbit->update_cbit(false, MOTORS_NODATA);});
    QObject::connect(iface, &DataInterface::received_targets, cbit, [cbit](){cbit->update_cbit(false, TEGRA_NODATA);});

    /** Create video interface **/
    VideoInterface* iface_v = new VideoInterface(1000);

    /** Connecting video timeout to renderer **/
    QObject::connect(iface_v, SIGNAL(video_timeout()), renderer, SLOT(on_video_timeout()));

    /** Create video procesor **/
    VideoProcessor* video_processor = new VideoProcessor();

    /** Connecting video update to video processor **/
    QObject::connect(iface_v, &VideoInterface::received_video, video_processor, [video_processor](image_msg image)
    {
        std::vector<char> data(image.data, image.data + image.len);
        cv::Mat img = cv::imdecode(cv::Mat(data), 1);

        video_processor->feed(img);
    });

    /** Connecting video update to tracker **/
    QObject::connect(iface_v, &VideoInterface::received_video, tracker, [tracker](image_msg image)
    {
        std::vector<char> data(image.data, image.data + image.len);
        cv::Mat img = cv::imdecode(cv::Mat(data), 1);

        tracker->on_camera_image(img);
    });
    /** Connecting video processor to renderer **/
    QObject::connect(video_processor, SIGNAL(frame_ready(cv::Mat)), renderer, SLOT(on_image(cv::Mat)));

    /** Connecting renderer to video processor (image algorithms abilitation) **/
    QObject::connect(renderer, SIGNAL(signal_clahe_changed_state(bool)), video_processor, SLOT(clahe(bool)));
    QObject::connect(renderer, SIGNAL(signal_polarity_changed_state(bool)), video_processor, SLOT(polarity(bool)));
    QObject::connect(renderer, SIGNAL(signal_denoise_changed_state(bool)), video_processor, SLOT(denoise(bool)));
    QObject::connect(renderer, &VideoRenderer::signal_b_filter_changed_state, video_processor, [video_processor](bool enabled){
        video_processor->channel_filter(enabled, 0);
    });
    QObject::connect(renderer, &VideoRenderer::signal_g_filter_changed_state, video_processor, [video_processor](bool enabled){
        video_processor->channel_filter(enabled, 1);
    });
    QObject::connect(renderer, &VideoRenderer::signal_r_filter_changed_state, video_processor, [video_processor](bool enabled){
        video_processor->channel_filter(enabled, 2);
    });

    /** Connecting video to cbit **/
    QObject::connect(iface_v, &VideoInterface::video_timeout, cbit, [cbit]() {cbit->update_cbit(true, VIDEO_NODATA);});
    QObject::connect(iface_v, &VideoInterface::received_video, cbit, [cbit]() {cbit->update_cbit(false, VIDEO_NODATA);});

    /** Connecting cbit result to video renderer **/
    QObject::connect(cbit, SIGNAL(cbit_result(quint32)), renderer, SLOT(update(quint32)));

    /** Create joystick handler **/
    JoystickInput* js_input = new JoystickInput(iface);

    /** Connecting joystick to cbit **/
    QObject::connect(js_input, &JoystickInput::js_failure, cbit, [cbit]() {cbit->update_cbit(true, JOYSTICK_NODATA);});
    QObject::connect(js_input, &JoystickInput::js_on, cbit, [cbit]() {cbit->update_cbit(false, JOYSTICK_NODATA);});

    /** Connecting joystick to video renderer **/
    QObject::connect(js_input, SIGNAL(right()), renderer, SLOT(show_context_menu()));
    QObject::connect(js_input, SIGNAL(left()), renderer, SLOT(hide_context_menu()));
    QObject::connect(js_input, SIGNAL(confirm()), renderer, SLOT(confirm_context_menu()));
    QObject::connect(js_input, &JoystickInput::up, renderer, [renderer](){renderer->navigate_context_menu(-1);});
    QObject::connect(js_input, &JoystickInput::down, renderer, [renderer](){renderer->navigate_context_menu(1);});
    QObject::connect(js_input, &JoystickInput::up, renderer, [renderer](){renderer->navigate_system_menu(-1);});
    QObject::connect(js_input, &JoystickInput::down, renderer, [renderer](){renderer->navigate_system_menu(1);});

    /** Start joystick **/
    js_input->start();

    /** Start tracker **/
    tracker->start();

    /** Launch Qt Application **/
    QObject::connect(renderer, SIGNAL(thread_quit()), js_input, SLOT(stop()));
    QObject::connect(js_input, SIGNAL(thread_quit()), &app, SLOT(quit()));

    return app.exec();
}
