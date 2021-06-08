#include "defs.h"
#include "data_server.h"
#include "video_server.h"
#include "video_renderer.h"
#include "logger.h"
#include "health_status.h"
#include "joystick.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>



int main(int argc, char** argv)
{
    printf("%s\n", PROJNAME);
    if (argc == 1)
    {
        printf("USAGE: %s [raspberry_address]\n", argv[0]);
    printf("%s\n\n", "Trying to use 192.168.43.83");
    }

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
    pid = fork();
    if(pid == 0)
    {
        /** child process joystick **/
        joystick_task(argc == 1 ? "192.168.43.83" : argv[1]);
        exit(EXIT_SUCCESS);
    }
    pid = fork();
    if(pid == 0)
    {
        /** child process health_status **/
        health_status(argc == 1 ? "192.168.43.83" : argv[1]);
        exit(EXIT_SUCCESS);
    }
    writelog("Servers running");

    init_window();

    cv::dnn::Net net = cv::dnn::readNetFromDarknet("/home/alex/darknet/cfg/yolov2.cfg", "/home/alex/darknet/yolo.weights");

    main_loop(argc == 1 ? "192.168.43.83" : argv[1], net);

    return 0;
}
