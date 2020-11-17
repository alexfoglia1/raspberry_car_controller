#include "defs.h"
#include "data_server.h"
#include "video_server.h"
#include "video_renderer.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void init_localsock(int* localsocket, struct sockaddr_in* localaddr, int port)
{
    *localsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(localaddr, 0x00, sizeof(struct sockaddr_in));
    localaddr->sin_family = AF_INET;
    localaddr->sin_port = htons(port);
    localaddr->sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(*localsocket, (struct sockaddr*)localaddr, sizeof(struct sockaddr));
}

void* imu_task(void* param)
{
    if(param == NULL)
    {
        int imu_socket;
        struct sockaddr_in saddr;
        init_localsock(&imu_socket, &saddr, IMUPORT);
        while(true)
        {
            imu_msg recv;
            socklen_t len;
            recvfrom(imu_socket, &recv, sizeof(imu_msg), 0, (struct sockaddr*)&saddr, &len);
            update_imu(recv);
        }
    }

    return NULL;
}

void* speed_task(void* param)
{
    if(param == NULL)
    {
        int speed_socket;
        struct sockaddr_in saddr;
        init_localsock(&speed_socket, &saddr, VELPORT);
        while(true)
        {
            speed_msg recv;
            socklen_t len;
            recvfrom(speed_socket, &recv, sizeof(speed_msg), 0, (struct sockaddr*)&saddr, &len);
            update_speed(recv);
        }
    }

    return NULL;
}

void* attitude_task(void* param)
{
    if(param == NULL)
    {
        int attitude_socket;
        struct sockaddr_in saddr;
        init_localsock(&attitude_socket, &saddr, ATTPORT);
        while(true)
        {
            attitude_msg recv;
            socklen_t len;
            recvfrom(attitude_socket, &recv, sizeof(attitude_msg), 0, (struct sockaddr*)&saddr, &len);
            update_attitude(recv);
        }
    }

    return NULL;
}

void* radiation_task(void* param)
{
    if(param == NULL)
    {
        int radiation_socket;
        struct sockaddr_in saddr;
        init_localsock(&radiation_socket, &saddr, RADPORT);
        while(true)
        {
            radiation_msg recv;
            socklen_t len;
            recvfrom(radiation_socket, &recv, sizeof(radiation_msg), 0, (struct sockaddr*)&saddr, &len);
            update_radiation(recv);
        }
    }

    return NULL;
}

int main(int argc, char** argv)
{
    printf("%s\n", PROJNAME);

    /** configure logfile **/
    char* logfile = "log.txt";
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

    pthread_t imu_thread, speed_thread, attitude_thread, radiation_thread;
    pthread_create(&imu_thread, NULL, imu_task, NULL);
    pthread_create(&speed_thread, NULL, speed_task, NULL);
    pthread_create(&attitude_thread, NULL, attitude_task, NULL);
    pthread_create(&radiation_thread, NULL, radiation_task, NULL);
    writelog("Threads running");

    pthread_join(imu_thread, NULL);
    pthread_join(speed_thread, NULL);
    pthread_join(attitude_thread, NULL);
    pthread_join(radiation_thread, NULL);

    return 0;
}
