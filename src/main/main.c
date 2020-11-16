#include "defs.h"
#include "data_transceiver.h"
#include "video_receiver.h"
#include "video_renderer.h"
#include "logger.h"

#include <stdio.h>


int main(int argc, char** argv)
{
    printf("%s\n", PROJNAME);

    init_dataserver();

    char buffer[256];
    msg_header* header = (msg_header*)(buffer);
    while(1)
    {
        bool ret = recv_data_from_board((char*)&header);
        if(ret)
        {
            uint32_t id = header->msg_id;
            int64_t sec = header->msg_timestamp.tv_sec;
            int64_t us  = header->msg_timestamp.tv_usec;
            char logprompt[256];
            sprintf(logprompt, "id: %d, sec: %ld usec: %ld\n", id, sec, us);
            writelog(stdout, logprompt);
        }

    }
}
