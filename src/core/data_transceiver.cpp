#include "data_transceiver.h"
#include "logger.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>

int data_server_sock;
int data_client_sock;
struct sockaddr_in data_saddr;
struct sockaddr_in data_daddr;


bool bind_data_server_sock()
{
    bool retval;

    memset(&data_saddr, 0x00, sizeof(struct sockaddr_in));
    data_saddr.sin_family = AF_INET;
    data_saddr.sin_port   = htons(DATPORT);
    data_saddr.sin_addr.s_addr = INADDR_ANY;

    memset(&data_daddr, 0x00, sizeof(struct sockaddr_in));
    data_daddr.sin_family = AF_INET;
    data_daddr.sin_port   = htons(DATPORT);
    data_daddr.sin_addr.s_addr = inet_addr(BOARD_ADDRESS);

    int sock_bind = bind(data_server_sock, (const struct sockaddr*)&data_saddr, sizeof(data_saddr));
    if(sock_bind)
    {
        retval = false;
        perror(ERR_BINDSOCKDATA);
        writelog(ERR_BINDSOCKDATA);
    }
    else
    {
        writelog(OK_BINDSOCKDATA);
        retval = true;
    }

    return retval;
}


bool init_dataserver()
{
    bool retval;

    int sock_creation_srv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int sock_creation_cli = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(!sock_creation_srv || !sock_creation_cli)
    {
        retval = false;
        perror(ERR_CREATESOCKDATA);
        writelog(ERR_CREATESOCKDATA);
    }
    else
    {
        writelog(OK_CREATESOCKDATA);
        data_server_sock = sock_creation_srv;
        data_client_sock = sock_creation_cli;

        const int optval[1] = {1};
        setsockopt(data_server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
        retval = bind_data_server_sock();
    }

    return retval;
}

bool recv_data_from_board(char* msg)
{
    bool retval;

    if(!data_server_sock)
    {
        writelog(ERR_UNINITIALIZED_SOCKET);
        retval = false;
    }
    else
    {
        writelog(OK_CAN_RECEIVE_DATA);
        size_t maximum_len = sizeof(imu_msg);
        socklen_t len;
        retval = recvfrom(data_server_sock, msg, maximum_len, 0, (struct sockaddr*)&data_saddr, &len);
        if(!retval)
        {
            writelog(ERR_RECVSOCKDATA);
        }
        else
        {
            writelog(OK_RECVSOCKDATA);
        }
    }

    return retval;
}

bool send_data_to_board(char* msg)
{
    bool retval;

    if(!data_client_sock)
    {
        writelog(ERR_UNINITIALIZED_SOCKET);
        retval = false;
    }
    else
    {
        writelog(OK_CANSENDDATA);
        size_t maximum_len = sizeof(command_msg);
        ssize_t send_res = sendto(data_client_sock, msg, maximum_len, 0, reinterpret_cast<struct sockaddr*>(&data_daddr), sizeof(struct sockaddr));
        if(send_res > 0)
        {
            writelog(OK_SENDDATA);
            retval = true;
        }
        else
        {
            writelog(ERR_SENDDATA);
            retval = false;
        }
    }

    return retval;
}
