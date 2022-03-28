#ifndef DATA_INTERFACE_H
#define DATA_INTERFACE_H

#include "defs.h"

#include <QUdpSocket>
#include <QTimer>
#include <semaphore.h>

class DataInterface : public QObject
{
    Q_OBJECT

public:
    DataInterface(int timeout_millis);

    void send_command(joystick_msg msg);

signals:
    void received_voltage(voltage_msg message);
    void received_attitude(attitude_msg message);
    void received_actuators(actuators_state_msg message);
    void received_addr(QString board_addr);
    void data_timeout(comp_t component);

private:
    QUdpSocket udp_socket;
    QHostAddress remote_address;
    QTimer voltage_timeout, imu_timeout, actuators_timeout;
    sem_t addr_sem;

private slots:
    void receive_data();
    void timeout_rx(comp_t component);

};


#endif //DATA_INTERFACE_H
