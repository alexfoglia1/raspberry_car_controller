#include "video_interface.h"

#include <QByteArray>

VideoInterface::VideoInterface()
{

    udp_socket.bind(UDP_PORT_VIDEO);

    connect(&udp_socket, SIGNAL(readyRead()), this, SLOT(receive_video()));

}

void VideoInterface::receive_video()
{
    if (udp_socket.hasPendingDatagrams())
    {
        quint64 size = udp_socket.pendingDatagramSize();
        char* bytes_in = new char[size];
        udp_socket.readDatagram(bytes_in, size);

        emit received_video(*reinterpret_cast<image_msg*>(bytes_in));

        delete[] bytes_in;
    }
}

