#include "data_transceiver.h"
#include "logger.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>


int server_sock;
struct sockaddr_in saddr;


bool bind_server_sock()
{
    bool retval;

    memset(&saddr, 0x00, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port   = htons(DATPORT);
    saddr.sin_addr.s_addr = INADDR_ANY;

    int sock_bind  = bind(server_sock, (const struct sockaddr*)&saddr, sizeof(saddr));
    if(sock_bind)
    {
        retval = false;
        perror(ERR_BINDSOCKDATA);
        writelog(stderr, ERR_BINDSOCKDATA);
    }
    else
    {
        writelog(stdout, OK_BINDSOCKDATA);
        retval = true;
    }

    return retval;
}

bool recvfrom_wrapper(char* msg, size_t maximum_len)
{
    bool retval;

    char buffer[maximum_len];
    socklen_t len;
    ssize_t sock_recv = recvfrom(server_sock, buffer, maximum_len, 0, (struct sockaddr*)&saddr, &len);
    if(!sock_recv)
    {
        retval = false;
        perror(ERR_RECVSOCKDATA);
    }
    else
    {
        msg_header* header_ptr = (msg_header*)(buffer);
        switch(header_ptr->msg_id)
        {
        case IMU_MSG_ID:
            memcpy((imu_msg*) msg, (imu_msg*) buffer, sizeof(imu_msg));
            retval = true;
            writelog(stdout, OK_IMU);
            break;
        case SPEED_MSG_ID:
            memcpy((speed_msg*) msg, (speed_msg*) buffer, sizeof(speed_msg));
            retval = true;
            writelog(stdout, OK_SPEED);
            break;
        case ATTITUDE_MSG_ID:
            memcpy((attitude_msg*) msg, (attitude_msg*) buffer, sizeof(attitude_msg));
            retval = true;
            writelog(stdout, OK_ATTITUDE);
            break;
        case RADIATION_MSG_ID:
            memcpy((radiation_msg*) msg, (radiation_msg*) buffer, sizeof(radiation_msg));
            retval = true;
            writelog(stdout, OK_RADIATION);
            break;
        default:
            retval = false;
            writelog(stderr, ERR_UNKNOWN_SOURCE);
            break;

        }
    }

    return retval;
}

bool init_dataserver()
{
    bool retval;

    int sock_creation = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(!sock_creation)
    {
        retval = false;
        perror(ERR_CREATESOCKDATA);
        writelog(stderr, ERR_CREATESOCKDATA);
    }
    else
    {
        writelog(stdout, OK_CREATESOCKDATA);
        server_sock = sock_creation;
        retval = bind_server_sock();
    }

    return retval;
}

bool recv_data_from_board(char* msg)
{
    bool retval;

    if(!server_sock)
    {
        writelog(stderr, ERR_UNINITIALIZED_SOCKET);
        retval = false;
    }
    else
    {
        writelog(stdout, OK_CAN_RECEIVE_DATA);
        size_t maximum_len = sizeof(imu_msg);
        retval = recvfrom_wrapper(msg, maximum_len);
    }

    return retval;
}

bool send_data_to_board(char* msg)
{

}
