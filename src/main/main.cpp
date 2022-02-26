#include <QApplication>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "defs.h"
#include "data_interface.h"
#include "video_interface.h"
#include "video_renderer.h"
#include "joystick.h"

const char* DEFAULT_RASPBY_ADDR = "192.168.1.20";

int main(int argc, char** argv)
{
    printf("%s\n", PROJNAME);
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

    /** Register meta types **/
    qRegisterMetaType<joystick_msg>();
    qRegisterMetaType<attitude_msg>();
    qRegisterMetaType<voltage_msg>();
    qRegisterMetaType<actuators_state_msg>();
    qRegisterMetaType<cbit_result_msg>();
    qRegisterMetaType<target_msg>();
    qRegisterMetaType<image_msg>();

    /** Create window render **/
    VideoRenderer* renderer = new VideoRenderer();

    /** Create data interface **/
    DataInterface* iface = new DataInterface();
    QObject::connect(iface, SIGNAL(received_attitude(attitude_msg)), renderer, SLOT(update(attitude_msg)));
    QObject::connect(iface, SIGNAL(received_voltage(voltage_msg)), renderer, SLOT(update(voltage_msg)));
    QObject::connect(iface, SIGNAL(received_cbit_result(cbit_result_msg)), renderer, SLOT(update(cbit_result_msg)));
    QObject::connect(iface, SIGNAL(received_actuators(actuators_state_msg)), renderer, SLOT(update(actuators_state_msg)));
    QObject::connect(iface, SIGNAL(received_targets(target_msg)), renderer, SLOT(update(target_msg)));

    /** Create video interface **/
    VideoInterface* iface_v = new VideoInterface();
    QObject::connect(iface_v, SIGNAL(received_video(image_msg)), renderer, SLOT(update(image_msg)));

    /** Start renerer **/
    renderer->init_window();
    renderer->start();

    /** Notify controller started **/
    joystick_msg controller_on = {{JS_ACC_MSG_ID}, 0x00, 0x00, 0x00, true, false};
    iface->send_command(controller_on, argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1]);

    /** Launch Qt Application **/
    app.exec();
}
