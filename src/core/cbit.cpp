#include "cbit.h"

Cbit::Cbit()
{
    act_cbit =  ARDUINO_NODATA  |
                ATTITUDE_NODATA |
                JOYSTICK_NODATA |
                MOTORS_NODATA   |
                TEGRA_NODATA    |
                VIDEO_NODATA;
}

void Cbit::update_cbit(bool active, quint32 cbit)
{
    act_cbit = (cbit & ARDUINO_NODATA)  ? active?  (ARDUINO_NODATA | act_cbit)  : (~ARDUINO_NODATA & act_cbit) :
               (cbit & ATTITUDE_NODATA) ? active?  (ATTITUDE_NODATA | act_cbit) : (~ATTITUDE_NODATA & act_cbit) :
               (cbit & JOYSTICK_NODATA) ? active?  (JOYSTICK_NODATA | act_cbit) : (~JOYSTICK_NODATA & act_cbit) :
               (cbit & MOTORS_NODATA)   ? active?  (MOTORS_NODATA | act_cbit)   : (~MOTORS_NODATA & act_cbit) :
               (cbit & TEGRA_NODATA)    ? active?  (TEGRA_NODATA | act_cbit)    : (~TEGRA_NODATA & act_cbit) :
               (cbit & VIDEO_NODATA)    ? active?  (VIDEO_NODATA | act_cbit)    : (~VIDEO_NODATA & act_cbit) : act_cbit;


    emit cbit_result(act_cbit);
}



