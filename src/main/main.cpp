#include "defs.h"
#include "data_server.h"
#include "video_server.h"
#include "video_renderer.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>

int main()
{
    printf("%s\n", PROJNAME);

    /** configure logfile **/
    const char* logfile = "log.txt";
    FILE* f = fopen(logfile, "w");
    fclose(f);
    configure(logfile);

    int pid = fork();
    if(pid == 0)
    {
        /** child process data_server **/
        data_task();
        exit(EXIT_SUCCESS);
    }

    pid = fork();
    if(pid == 0)
    {
        /** child process video_server **/
        video_task();
        exit(EXIT_SUCCESS);
    }
    writelog("Servers running");

    init_window();
    main_loop();
    return 0;
}
