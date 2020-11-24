#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/defs.h"
#include <string.h>

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in saddr;
    memset(&saddr, 0x00, sizeof(struct sockaddr_in));
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(1234);
    bind(s, (const struct sockaddr*)&saddr, sizeof(saddr));
    while(1)
    {
            msg_header* hdr;
            imu_msg imu_out;
            socklen_t len;
            recvfrom(s, &imu_out, sizeof(imu_msg), 0, (struct sockaddr*)&saddr, &len);
            std::cout << "out from recv" << std::endl;
            hdr = reinterpret_cast<msg_header*>(&imu_out);
            std::cout << hdr->msg_id;
            if(imu_out.header.msg_id == IMU_MSG_ID)
            {
            std::cout << "Angular speed: " << imu_out.gyrox << "\t" << imu_out.gyroy << std::endl;
            std::cout << "Acc: " << imu_out.ax << "\t" << imu_out.ay << "\t" << imu_out.az << std::endl;
            //std::cout << "Speed: " << speed_out.vx << "\t" << speed_out.vy << "\t" << speed_out.vz << std::endl;
            //std::cout << "Attitude: " << att_out.roll << "\t" << att_out.pitch << std::endl << std::endl;
            }
    }
}
