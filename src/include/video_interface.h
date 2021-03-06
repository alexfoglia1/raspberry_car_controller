#ifndef VIDEO_INTERFACE_H
#define VIDEO_INTERFACE_H

#include "defs.h"

#include <QUdpSocket>
#include <QTimer>
#include <opencv2/opencv.hpp>

class VideoInterface : public QObject
{
    Q_OBJECT

public:
    VideoInterface(int timeout_millis);

signals:
    void received_video(cv::Mat image);
    void video_timeout();

private:
    QUdpSocket udp_socket;
    QString remote_address;
    QTimer recv_timer;

private slots:
    void receive_video();
    void on_recv_timeout();

};


#endif //VIDEO_INTERFACE_H
