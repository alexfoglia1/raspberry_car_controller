#include <QApplication>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "defs.h"
#include "cbit.h"
#include "data_interface.h"
#include "video_interface.h"
#include "video_renderer.h"
#include "joystick.h"

const char* DEFAULT_RASPBY_ADDR = "192.168.1.24";

int main(int argc, char** argv)
{
    printf("%s v%s.%s\n", APP_NAME, MAJOR_VERS, MINOR_VERS);
    if (argc == 1)
    {
        printf("USAGE: %s [raspberry_address]\n", argv[0]);
        printf("Trying to use %s\n\n", DEFAULT_RASPBY_ADDR);
    }

    /** Speed test file **/
    FILE* f = fopen("speedtest.csv", "w");
    fprintf(f, "dt, avg_vin, avg_vout\n");
    fclose(f);

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

    /** Create cbit instance **/
    Cbit* cbit = new Cbit();

    /** Create window render **/
    VideoRenderer* renderer = new VideoRenderer();

    /** Start renerer **/
    renderer->init_window();
    renderer->start();

    /** Create data interface **/
    DataInterface* iface = new DataInterface(argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1]);
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
        break;
    }
    });
    QObject::connect(iface, &DataInterface::received_attitude, cbit, [cbit](){cbit->update_cbit(false, ATTITUDE_NODATA);});
    QObject::connect(iface, &DataInterface::received_voltage, cbit, [cbit](){cbit->update_cbit(false, ARDUINO_NODATA);});
    QObject::connect(iface, &DataInterface::received_actuators, cbit, [cbit](){cbit->update_cbit(false, MOTORS_NODATA);});
    QObject::connect(iface, &DataInterface::received_targets, cbit, [cbit](){cbit->update_cbit(false, TEGRA_NODATA);});

    /** Create video interface **/
    VideoInterface* iface_v = new VideoInterface(1000);

    /** Connecting video updates to renderer **/
    QObject::connect(iface_v, SIGNAL(received_video(image_msg)), renderer, SLOT(update(image_msg)));
    QObject::connect(iface_v, SIGNAL(video_timeout()), renderer, SLOT(clear()));

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

    /** Start joystick **/
    js_input->start();

    /** Launch Qt Application **/
    QObject::connect(renderer, SIGNAL(thread_quit()), js_input, SLOT(stop()));
    QObject::connect(js_input, SIGNAL(thread_quit()), &app, SLOT(quit()));
    return app.exec();
}
