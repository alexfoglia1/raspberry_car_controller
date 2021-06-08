#include "health_status.h"
#include "defs.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void health_status(const char* board_address)
{
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in daddr;

    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(HLTPORT);
    daddr.sin_addr.s_addr = inet_addr(board_address);

    float delay_s = 1.0/HEALTH_STATUS_RATE;
    int delay_us = static_cast<int>(delay_s * 1e6);

    while (true)
    {
        health_status_msg status;
        status.header.msg_id = HLT_MSG_ID;
        status.whoami = WHOAMI_PC;

        ssize_t res = sendto(s, (char*) &status, sizeof(health_status_msg), 0, reinterpret_cast<struct sockaddr*>(&daddr), sizeof(struct sockaddr));
        if (!res)
        {
            perror("Health status");
        }

        usleep(delay_us);
    }

}
