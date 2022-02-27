#ifndef CBIT_H
#define CBIT_H
#include "defs.h"
#include <QObject>

const quint32 TEGRA_NODATA    = 0x01;
const quint32 ATTITUDE_NODATA = 0x02;
const quint32 VIDEO_NODATA    = 0x04;
const quint32 JOYSTICK_NODATA = 0x08;
const quint32 ARDUINO_NODATA  = 0x10;
const quint32 MOTORS_NODATA   = 0x20;

class Cbit : public QObject
{
    Q_OBJECT
public:
    Cbit();

public slots:
    void update_cbit(bool active, quint32 cbit);

signals:
    void cbit_result(quint32);

private:
    quint32 act_cbit;
};

#endif // CBIT_H
