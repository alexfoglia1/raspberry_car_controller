#include "keymap.h"
#include "keyboard_input.h"

#include <opencv2/opencv.hpp>


KeyboardInput::KeyboardInput()
{

}

void KeyboardInput::get_keyboard_event()
{
    int key = cv::waitKey(0);
#if 0
    switch (key)
    {
    case TOGGLE_SPEED:
        controller->postEvent(Controller::controller_event_t::received_toggle_speed);
        break;
    case TOGGLE_LOS:
        controller->postEvent(Controller::controller_event_t::received_toggle_los);
        break;
    case TOGGLE_MENU:
        controller->postEvent(Controller::controller_event_t::received_toggle_help);
        break;
    case TOGGLE_SS:
        controller->postEvent(Controller::controller_event_t::received_toggle_system_status);
        break;
    case TOGGLE_JS:
        controller->postEvent(Controller::controller_event_t::received_toggle_js_help);
        break;
    case VIDEOREC:
        controller->postEvent(Controller::controller_event_t::received_toggle_videorec);
        break;
    case DEV_MODE:
        controller->postEvent(Controller::controller_event_t::received_toggle_devmode);
        break;
    case UP_ARROW:
        controller->postEvent(Controller::controller_event_t::pressed_up);
        break;
    case DOWN_ARROW:
        controller->postEvent(Controller::controller_event_t::pressed_down);
        break;
    case LEFT_ARROW:
        controller->postEvent(Controller::controller_event_t::pressed_left);
        break;
    case RIGHT_ARROW:
        controller->postEvent(Controller::controller_event_t::pressed_right);
        break;
    case ESCAPE:
        controller->postEvent(Controller::controller_event_t::stop_controller);
        break;
    }
#endif
}
