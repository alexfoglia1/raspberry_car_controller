#include "data_interface.h"

#include <QByteArray>
#include <QHostAddress>

DataInterface::DataInterface(int timeout_millis)
{
    sem_init(&addr_sem, 0, 1);

    udp_socket.bind(UDP_PORT);
    voltage_timeout.setInterval(timeout_millis);
    imu_timeout.setInterval(timeout_millis);
    actuators_timeout.setInterval(timeout_millis);

    connect(&udp_socket, SIGNAL(readyRead()), this, SLOT(receive_data()));
    connect(&voltage_timeout, &QTimer::timeout, this, [this]() {timeout_rx(comp_t::ARDUINO);});
    connect(&imu_timeout, &QTimer::timeout, this, [this]() {timeout_rx(comp_t::ATTITUDE);});
    connect(&actuators_timeout, &QTimer::timeout, this, [this]() {timeout_rx(comp_t::MOTORS);});

    voltage_timeout.start();
    imu_timeout.start();
    actuators_timeout.start();
}

void DataInterface::timeout_rx(comp_t component)
{
    emit data_timeout(component);
}

void DataInterface::send_command(joystick_msg msg)
{
    QByteArray datagram;
    datagram.setRawData(reinterpret_cast<char*>(&msg), sizeof(joystick_msg));

    sem_wait(&addr_sem);
    udp_socket.writeDatagram(datagram, remote_address, UDP_PORT);
    sem_post(&addr_sem);
}

void DataInterface::receive_data()
{
    if (udp_socket.hasPendingDatagrams())
    {
       quint64 size = udp_socket.pendingDatagramSize();
       char* bytes_in = new char[size];

       quint16 remote_port; //unused
       sem_wait(&addr_sem);
       udp_socket.readDatagram(bytes_in, size, &remote_address, &remote_port);
       sem_post(&addr_sem);

       QString str_rem_addr = remote_address.toString();
       QStringList strlist_rem_addr = str_rem_addr.split(":");
       quint64 listlen = strlist_rem_addr.size();
       emit received_addr(strlist_rem_addr.at(listlen - 1));

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

