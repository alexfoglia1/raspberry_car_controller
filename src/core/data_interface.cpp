#include "data_interface.h"

#include <QByteArray>
#include <QHostAddress>

DataInterface::DataInterface(QString address)
{
    udp_socket.bind(UDP_PORT);
    remote_address = address;
    voltage_timeout.setInterval(1000);
    imu_timeout.setInterval(1000);
    actuators_timeout.setInterval(1000);
    tegra_timeout.setInterval(1000);

    connect(&udp_socket, SIGNAL(readyRead()), this, SLOT(receive_data()));
    connect(&voltage_timeout, &QTimer::timeout, this, [this]() {timeout_rx(comp_t::ARDUINO);});
    connect(&imu_timeout, &QTimer::timeout, this, [this]() {timeout_rx(comp_t::ATTITUDE);});
    connect(&actuators_timeout, &QTimer::timeout, this, [this]() {timeout_rx(comp_t::MOTORS);});
    connect(&tegra_timeout, &QTimer::timeout, this, [this]() {timeout_rx(comp_t::TEGRA);});

    voltage_timeout.start();
    imu_timeout.start();
    actuators_timeout.start();
    tegra_timeout.start();
}

void DataInterface::timeout_rx(comp_t component)
{
    emit data_timeout(component);
}

void DataInterface::send_command(joystick_msg msg)
{
    QByteArray datagram;
    datagram.setRawData(reinterpret_cast<char*>(&msg), sizeof(joystick_msg));
    QHostAddress address(remote_address);
    qDebug("thr state out(%d)\n", msg.throttle_state);
    udp_socket.writeDatagram(datagram, address, UDP_PORT);
}

void DataInterface::receive_data()
{
    if (udp_socket.hasPendingDatagrams())
    {
       quint64 size = udp_socket.pendingDatagramSize();
       char* bytes_in = new char[size];
       udp_socket.readDatagram(bytes_in, size);

       msg_header* header = reinterpret_cast<msg_header*>(bytes_in);
       switch(header->msg_id)
       {
       case ATTITUDE_MSG_ID:
       {
           imu_timeout.stop();
           attitude_msg* msg_in = reinterpret_cast<attitude_msg*>(bytes_in);
           emit received_attitude(*msg_in);
           imu_timeout.start();
           break;
       }
       case VOLTAGE_MSG_ID:
       {
           voltage_timeout.stop();
           voltage_msg* msg_in = reinterpret_cast<voltage_msg*>(bytes_in);
           emit received_voltage(*msg_in);
           voltage_timeout.start();
           break;
       }
       case TARGET_MSG_ID:
       {
           tegra_timeout.stop();
           target_msg* msg_in = reinterpret_cast<target_msg*>(bytes_in);
           emit received_targets(*msg_in);
           tegra_timeout.start();
           break;
       }
       case ACTUATORS_STATE_MSG_ID:
       {
           actuators_timeout.stop();
           actuators_state_msg* msg_in = reinterpret_cast<actuators_state_msg*>(bytes_in);
           emit received_actuators(*msg_in);
           actuators_timeout.start();
           break;
       }
       default:
           qWarning("Received unknown msg id(%d)\n", header->msg_id);
           break;
       }

        delete[] bytes_in;
    }
}

