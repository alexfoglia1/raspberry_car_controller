#ifndef VIDEO_INTERFACE_H
#define VIDEO_INTERFACE_H

#include "defs.h"

#include <QUdpSocket>

class VideoInterface : public QObject
{
    Q_OBJECT

public:
    VideoInterface();

signals:
    void received_video(image_msg message);

private:
    QUdpSocket udp_socket;

private slots:
    void receive_video();

};


#endif //VIDEO_INTERFACE_H
