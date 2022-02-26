#include "data_interface.h"

#include <QByteArray>
#include <QHostAddress>

DataInterface::DataInterface()
{

    udp_socket.bind(UDP_PORT);

    connect(&udp_socket, SIGNAL(readyRead()), this, SLOT(receive_data()));

}

void DataInterface::send_command(joystick_msg msg, QString remote_address)
{
    QByteArray datagram(reinterpret_cast<char*>(&msg));
    QHostAddress address(remote_address);
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
           attitude_msg* msg_in = reinterpret_cast<attitude_msg*>(bytes_in);
           emit received_attitude(*msg_in);
           break;
       }
       case VOLTAGE_MSG_ID:
       {
           voltage_msg* msg_in = reinterpret_cast<voltage_msg*>(bytes_in);
           emit received_voltage(*msg_in);
           break;
       }
       case TARGET_MSG_ID:
       {
           target_msg* msg_in = reinterpret_cast<target_msg*>(bytes_in);
           emit received_targets(*msg_in);
           break;
       }
       case ACTUATORS_STATE_MSG_ID:
       {
           actuators_state_msg* msg_in = reinterpret_cast<actuators_state_msg*>(bytes_in);
           emit received_actuators(*msg_in);
           break;
       }
       case CBITRES_MSG_ID:
       {
           cbit_result_msg* msg_in = reinterpret_cast<cbit_result_msg*>(bytes_in);
           emit received_cbit_result(*msg_in);
           break;
       }
       default:
           qWarning("Received unknown msg id(%d)\n", header->msg_id);
           break;
       }

        delete[] bytes_in;
    }
}

