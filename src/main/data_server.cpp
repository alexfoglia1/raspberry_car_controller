#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "data_server.h"
#include "data_transceiver.h"
#include "defs.h"
#include "logger.h"

void data_task()
{
    init_dataserver();

    int sock_to_parent = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in daddr;
    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char buffer[sizeof(imu_msg)];
    while(true)
    {
         bool ret = recv_data_from_board(buffer);
         msg_header* header_ptr = (msg_header*)(&buffer[0]);
         if(ret)
         {
             switch(header_ptr->msg_id)
             {
             case IMU_MSG_ID:
                 daddr.sin_port = htons(IMUPORT);
                 sendto(sock_to_parent, buffer, sizeof(imu_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
                 writelog(OK_IMU);
                 break;
             case SPEED_MSG_ID:
                 daddr.sin_port = htons(VELPORT);
                 sendto(sock_to_parent, buffer, sizeof(speed_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
                 writelog(OK_SPEED);
                 break;
             case ATTITUDE_MSG_ID:
                 daddr.sin_port = htons(ATTPORT);
                 sendto(sock_to_parent, buffer, sizeof(attitude_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
                 writelog(OK_ATTITUDE);
                 break;
             case RADIATION_MSG_ID:
                 daddr.sin_port = htons(RADPORT);
                 sendto(sock_to_parent, buffer, sizeof(speed_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
                 writelog(OK_RADIATION);
                 break;
             default:
                 writelog(ERR_UNKNOWN_SOURCE);
                 break;
             }
        }
    }

    return;
}
