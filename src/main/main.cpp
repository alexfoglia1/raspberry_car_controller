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

const char* DEFAULT_RASPBY_ADDR = "127.0.0.1";

int main(int argc, char** argv)
{
    printf("%s\n", PROJNAME);
    if (argc == 1)
    {
        printf("USAGE: %s [raspberry_address]\n", argv[0]);
        printf("Trying to use %s\n\n", DEFAULT_RASPBY_ADDR);
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
        joystick_task(argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1]);
        exit(EXIT_SUCCESS);
    }
    pid = fork();
    if(pid == 0)
    {
        /** child process health_status **/
        health_status(argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1]);
        exit(EXIT_SUCCESS);
    }
    writelog("Servers running");

    init_window();

#ifdef MACHINE_LEARNING
    cv::dnn::Net net = cv::dnn::readNetFromDarknet("/home/alex/darknet/cfg/yolov2.cfg", "/home/alex/darknet/yolo.weights");
#else
    cv::dnn::Net net;
#endif
    main_loop(argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1], net);

    return 0;
}
