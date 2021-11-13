#include "defs.h"
#include "video_renderer.h"
#include "joystick.h"
#include "cbit.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/stat.h>
#include <experimental/filesystem>

const char* DEFAULT_RASPBY_ADDR = "192.168.1.13";

int main(int argc, char** argv)
{
    printf("%s\n", PROJNAME);
    if (argc == 1)
    {
        printf("USAGE: %s [raspberry_address]\n", argv[0]);
        printf("Trying to use %s\n\n", DEFAULT_RASPBY_ADDR);
    }

    /** Speed test file **/
    FILE* f = fopen("speedtest.csv", "w");
    fprintf(f, "dt, avg_vin, avg_vout\n");
    fclose(f);
    int pid = fork();
    if(pid == 0)
    {
        /** child process joystick **/
        joystick_task(argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1]);
        exit(EXIT_SUCCESS);
    }

    pid = fork();
    if(pid == 0)
    {
        /** child process cbit **/
        cbit_task();
        exit(EXIT_SUCCESS);
    }

    init_window();

    main_loop(argc == 1 ? DEFAULT_RASPBY_ADDR : argv[1]);

    return 0;
}
