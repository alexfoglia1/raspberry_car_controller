#include "video_interface.h"

#include <QByteArray>

VideoInterface::VideoInterface(int timeout_millis)
{

    udp_socket.bind(UDP_PORT_VIDEO);
    connect(&udp_socket, SIGNAL(readyRead()), this, SLOT(receive_video()));
    connect(&recv_timer, SIGNAL(timeout()), this, SLOT(on_recv_timeout()));

    recv_timer.setInterval(timeout_millis);
    recv_timer.start();
}

void VideoInterface::receive_video()
{
    if (udp_socket.hasPendingDatagrams())
    {
        recv_timer.stop();

        quint64 size = udp_socket.pendingDatagramSize();
        char* bytes_in = new char[size];
        udp_socket.readDatagram(bytes_in, size);

        emit received_video(*reinterpret_cast<image_msg*>(bytes_in));

        delete[] bytes_in;

        recv_timer.start();
    }
}

void VideoInterface::on_recv_timeout()
{
    emit video_timeout();
}

