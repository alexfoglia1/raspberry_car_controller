#include "defs.h"
#include "data_server.h"
#include "video_server.h"
#include "video_renderer.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    printf("%s\n", PROJNAME);

    /** configure logfile **/
    char* logfile = "log.txt";
    FILE* f = fopen(logfile, "w");
    fclose(f);
    //configure(logfile);

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

    /** setup opengl **/
    init_window(argc, argv);
    show_window();

    return 0;
}
