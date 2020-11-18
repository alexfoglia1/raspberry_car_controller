#include "video_server.h"
#include "video_receiver.h"
#include "defs.h"
#include "logger.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

void video_task()
{
    init_videoserver();

    int sock_to_parent = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in daddr;
    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    daddr.sin_port = htons(RENPORT);

    char* buffer = malloc(sizeof(image_msg));
    while(true)
    {
         bool ret = recv_video_from_board(buffer);
         if(ret)
         {
             sendto(sock_to_parent, buffer, sizeof(image_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
             writelog(OK_IMAGE);
        }
    }

    return;
}
