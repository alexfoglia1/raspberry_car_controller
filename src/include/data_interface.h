#ifndef DATA_INTERFACE_H
#define DATA_INTERFACE_H

#include "defs.h"

#include <QUdpSocket>

class DataInterface : public QObject
{
    Q_OBJECT

public:
    DataInterface();

    void send_command(joystick_msg msg);

signals:
    void received_voltage(voltage_msg message);
    void received_attitude(attitude_msg message);
    void received_actuators(actuators_state_msg message);
    void received_targets(target_msg message);
    void received_cbit_result(cbit_result_msg message);

private:
    QUdpSocket udp_socket;

private slots:
    void receive_data();

};


#endif //DATA_INTERFACE_H
