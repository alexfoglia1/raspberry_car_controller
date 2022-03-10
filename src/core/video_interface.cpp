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
    char bytes[MAX_IMAGESIZE + 2];
    if (udp_socket.hasPendingDatagrams())
    {
        recv_timer.stop();

        quint64 size = udp_socket.pendingDatagramSize();
        if (size <= MAX_IMAGESIZE + 2)
        {
            udp_socket.readDatagram(bytes, size);
            image_msg image = *reinterpret_cast<image_msg*>(bytes);
            std::vector<char> data(image.data, image.data + image.len);
            cv::Mat img = cv::imdecode(cv::Mat(data), 1);

            emit received_video(img);

            recv_timer.start();
        }
    }
}

void VideoInterface::on_recv_timeout()
{
    emit video_timeout();
}

