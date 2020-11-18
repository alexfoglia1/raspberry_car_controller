#include "../include/defs.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/random.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

double random_unif(double a, double b, uint32_t repeat)
{
    if(a > b)
    {
        double temp = a;
        a = b;
        b = temp;
    }
    double randoms[repeat];
    for(uint32_t i = 0; i < repeat; i++)
    {
        int64_t rndi = rand();
        double normalized_rndi = rndi / (double) RAND_MAX;
        double expanded_normal = normalized_rndi * (b - a);
         
        randoms[i] = a + expanded_normal;
    }
    
    double sum = 0.0;
    for(uint32_t i = 0; i < repeat; i++)
    {
        sum += randoms[i];
    }
    
    return sum / (double) repeat;
}

int main()
{
    srand(time(NULL));
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in daddr, vaddr;
    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    memset(&vaddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    daddr.sin_port = htons(DATPORT);
    vaddr.sin_family = AF_INET;
    vaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    vaddr.sin_port = htons(VIDPORT);
    
    image_msg video_out;
    for(int i = 0; i < SCREEN_ROWS; i++)
    {
        for(int j = 0; j < SCREEN_COLS; j++)
        {
            video_out.image[i][j][0] = 25;
            video_out.image[i][j][1] = 50;
            video_out.image[i][j][2] = 75;
        }
    }
    
    while(1)
    {
        imu_msg imu;
        gettimeofday(&imu.header.msg_timestamp, NULL);
        imu.header.msg_id = IMU_MSG_ID;
        imu.ax = 0.01 + random_unif(-1.0, 1.0, 10);
        imu.ay = -0.01 + random_unif(-1.0, 1.0, 10);;
        imu.az = -9.81 + random_unif(-1.0, 1.0, 10);;
        imu.gyrox = 0.02 + random_unif(-1.0, 1.0, 10);
        imu.gyroy = -0.02 + random_unif(-1.0, 1.0, 10);
        imu.gyroz = 0.00 + random_unif(-1.0, 1.0, 10);
        imu.magnx = 0.01 + random_unif(-1.0, 1.0, 10);
        imu.magny = 0.02 + random_unif(-1.0, 1.0, 10);
        imu.magnz = 0.03 + random_unif(-1.0, 1.0, 10);
        
        speed_msg speed;
        gettimeofday(&speed.header.msg_timestamp, NULL);
        speed.header.msg_id = SPEED_MSG_ID;
        speed.vx = 20.0 + random_unif(-3.0, 3.0, 10);
        speed.vy = 28.0 + random_unif(-3.0, 3.0, 10);
        speed.vz = 9.0 + random_unif(-3.0, 3.0, 10);
        
        attitude_msg attitude;
        gettimeofday(&attitude.header.msg_timestamp, NULL);
        attitude.header.msg_id = ATTITUDE_MSG_ID;
        attitude.roll = 1.15 + random_unif(-0.05, 0.05, 10);
        attitude.pitch = 0.17 + random_unif(-0.01, 0.01, 10);
        attitude.yaw = 0.5 + random_unif(-0.1, 0.1, 10);
        
        radiation_msg rad;
        gettimeofday(&rad.header.msg_timestamp, NULL);
        rad.header.msg_id = RADIATION_MSG_ID;
        rad.CPM = 7518.2 + random_unif(-10.0, 10.0, 10);
        rad.uSv_h = 45.1 + random_unif(-5.0, 5.0, 10);
        
        for(int i = 0; i < sizeof(image_msg); i+=1024)
        {
            char* pBuf = (char*)&video_out;
            sendto(sock, pBuf + i, 1024, 0, (struct sockaddr*)&vaddr, sizeof(vaddr));
            perror("sendto video");
        }
        sendto(sock, (char*)&imu, sizeof(imu_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
        sendto(sock, (char*)&speed, sizeof(speed_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
        sendto(sock, (char*)&attitude, sizeof(attitude_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));
        sendto(sock, (char*)&rad, sizeof(radiation_msg), 0, (struct sockaddr*)&daddr, sizeof(daddr));

        
        usleep(200000);
    }
    
    return 0;
}
