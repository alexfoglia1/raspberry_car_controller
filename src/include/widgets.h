#ifndef WIDGETS_H
#define WIDGETS_H

#include "cbit.h"
#include "defs.h"
#include <opencv2/opencv.hpp>
#include <stdint.h>
#include <map>
#include <vector>


namespace widgets
{
    class ContextMenu
    {
    public:
        enum ContextMenuItem
        {
            DEHAZE,
            LACE,
            ALGORITMO_CHE_NON_ESISTE,
            DIO_BOIA,
            ALCUNE_COSE,
            PRESCRITTI_CARTONATI,
            VIOLA_MERDA
        };

        typedef struct
        {
            ContextMenuItem item;
            QString text;
        } ContextMenuItemKey;

        ContextMenu();
        std::vector<ContextMenuItemKey> items = {{DEHAZE, "DEHAZE"},
                                                 {LACE, "LACE"},
                                                 {ALGORITMO_CHE_NON_ESISTE, "ALGORITMO ALCYONE"},
                                                 {DIO_BOIA, "DIO BOIA"},
                                                 {ALCUNE_COSE, "ALCUNE COSE"},
                                                 {PRESCRITTI_CARTONATI, "PRESCRITTI CARTONATI"},
                                                 {VIOLA_MERDA, "VIOLA MERDA PUMPUM"},
                                                };
        bool visible;
        int selected_item;

        void hide();
        void show();
        void navigate(int delta);
        void select();
        void draw(cv::Mat* frame);
    };

    extern const int THROTTLESTATE;
    extern const int LOS;
    extern const int TARGETS;
    extern const int HELP;
    extern const int VIDEO_REC;
    extern const int SYSTEM_STATUS;
    extern const int JS_HELP;
    extern const int DEVELOPER_MODE;

    extern bool* is_enabled;

    namespace throttlestate
    {
        extern char* throttle_display;
        extern char* speed_display_kmh;
        extern char* speed_display_ms;
        extern float duty_cycle;

        void init();
        void updateThrottle(uint8_t throttle_state);
        void updateVoltageIn(float voltage_in);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace los
    {
        extern int H_FOV_DEG;
        extern int LOS_RAY;

        extern float act_yaw_deg;
        extern float act_pitch_deg;
        extern float act_roll_deg;

        void init();
        void update(double pitch, double roll, double yaw);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace targets
    {
        extern target_data* active_targets;
        extern int active_size;

        void init();
        void update(target_msg msg);
        void draw(cv::Mat* imagewindow);
    }

    namespace help
    {
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace js_help
    {
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace videorec
    {
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace systemstatus
    {
        enum motor_status_t
        {
            IDLE = 0x00,
            RUNNING = 0x01,
            FAILURE = 0x02
        };

        extern bool tegra_status;
        extern bool att_status;
        extern bool vid_status;
        extern bool js_status;
        extern bool arduino_status;
        extern float motor_voltage_in;
        extern float motor_voltage_out;
        extern float duty_cycle;
        extern motor_status_t motor_status;

        void init();
        void updateCbit(quint32 cbit);
        void updateMotorVoltageIn(double voltage);
        void updateMotorVoltageOut(uint8_t pwm);
        void updateRemoteSystemState(uint8_t system_state);
        void draw(cv::Mat* imagewindow, int x, int y);
    }

    namespace devmode
    {
        class DeveloperMode
        {
        public:

            enum Tab
            {
                ATTITUDE,
                VOLTAGE,
                DETECTOR,
                SPEEDTEST,
                EDITPARAMS
            };

            enum DeveloperModeEvent
            {
                TAB_LEFT,
                TAB_RIGHT,
                ITEM_UP,
                ITEM_DOWN,
                ITEM_CONFIRM,
                ITEM_VALUE_UP,
                ITEM_VALUE_DOWN
            };

            enum TabItem
            {
                START_STOP_SPEED_TEST,
                KEYBOARD_RX_TIMEOUT,
                SOCKET_RX_TIMEOUT,
                VIDEOREC_FPS,
                IMU_RECALIBRATE
            };

            typedef struct
            {
                Tab activeTab;
                TabItem currentTabItem;
                DeveloperModeEvent event;
                Tab nextTab;
                Tab nextTabItem;
            } DeveloperModeStateMachineEntry;

            DeveloperMode();
        private:

        };
    }
}


#endif // WIDGETS_H
