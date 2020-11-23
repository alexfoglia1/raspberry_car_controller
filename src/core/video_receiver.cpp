#include "video_receiver.h"
#include "defs.h"
#include "logger.h"

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

int video_server_sock;
struct sockaddr_in video_saddr;

bool bind_video_server_sock()
{
    bool retval;

    memset(&video_saddr, 0x00, sizeof(struct sockaddr_in));
    video_saddr.sin_family = AF_INET;
    video_saddr.sin_port   = htons(VIDPORT);
    video_saddr.sin_addr.s_addr = INADDR_ANY;

    int sock_bind  = bind(video_server_sock, (const struct sockaddr*)&video_saddr, sizeof(video_saddr));
    if(sock_bind)
    {
        retval = false;
        perror(ERR_BINDSOCKVIDEO);
        writelog(ERR_BINDSOCKVIDEO);
    }
    else
    {
        writelog(OK_BINDSOCKVIDEO);
        retval = true;
    }

    return retval;
}


bool init_videoserver()
{
    bool retval;

    int sock_creation = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(!sock_creation)
    {
        retval = false;
        perror(ERR_CREATESOCKVIDEO);
        writelog(ERR_CREATESOCKVIDEO);
    }
    else
    {
        writelog(OK_CREATESOCKVIDEO);
        video_server_sock = sock_creation;
        const int optval[1] = {1};
        setsockopt(video_server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
        retval = bind_video_server_sock();
    }

    return retval;
}

bool recv_video_from_board(char* msg)
{
    bool retval;

    if(!video_server_sock)
    {
        writelog(ERR_UNINITIALIZED_SOCKET);
        retval = false;
    }
    else
    {

        writelog(OK_CAN_RECEIVE_VIDEO);
        size_t maximum_len = sizeof(image_msg);
        socklen_t len;
        retval = recvfrom(video_server_sock, msg, maximum_len, 0, (struct sockaddr*)&video_saddr, &len);
        if(!retval)
        {
            writelog(ERR_RECVSOCKVIDEO);
        }
        else
        {
            writelog(OK_RECVSOCKVIDEO);
        }
    }

    return retval;
}
