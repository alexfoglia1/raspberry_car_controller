#ifndef DATA_TRANSCEIVER_H
#define DATA_TRANSCEIVER_H
#include "defs.h"
#include <stdbool.h>

bool init_dataserver();
bool recv_data_from_board(char* msg);
bool send_data_to_board(char* msg, const char* board_address);


#endif //DATA_TRANSCEIVER_H
