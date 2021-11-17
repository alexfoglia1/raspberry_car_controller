#include "cbit.h"
#include "defs.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

bool updated(cbit_result_msg m1, cbit_result_msg m2)
{
    return
            m1.tegra_failure != m2.tegra_failure ||
            m1.js_failure    != m2.js_failure    ||
            m1.att_failure   != m2.att_failure   ||
            m1.vid_failure   != m2.vid_failure;
}

void __attribute__((noreturn)) cbit_task()
{
    int rxsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int txsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in saddr, daddr;
    memset(&saddr, 0x00, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(BITPORT);

    memset(&daddr, 0x00, sizeof(struct sockaddr_in));
    daddr.sin_family = AF_INET;
    daddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    daddr.sin_port = htons(BITRESPORT);

    bind(rxsock, reinterpret_cast<struct sockaddr*>(&saddr), sizeof(struct sockaddr_in));
    cbit_result_msg tx;
    memset(&tx, 0x01, sizeof(cbit_result_msg)); /** Init all failures **/
    tx.header.msg_id = CBITRES_MSG_ID;

    cbit_result_msg last_tx;
    memcpy(&last_tx, &tx, sizeof(cbit_result_msg));

    while (true)
    {
        cbit_msg rx;
        if (recv(rxsock, reinterpret_cast<char*>(&rx), sizeof(cbit_msg), 0) > 0)
        {
            switch(rx.component)
            {
            case comp_t::TEGRA:
                tx.tegra_failure = rx.is_failure;
                break;
            case comp_t::ATTITUDE:
                tx.att_failure = rx.is_failure;
                break;
            case comp_t::JOYSTICK:
                tx.js_failure = rx.is_failure;
                break;
            case comp_t::VIDEO:
                tx.vid_failure = rx.is_failure;
                break;
            case comp_t::ARDUINO:
                tx.arduino_failure = rx.is_failure;
                break;
            default:
                break;
            }

            if(updated(tx, last_tx))
            {
                sendto(txsock, &tx, sizeof(cbit_result_msg), 0, (struct sockaddr*)(&daddr), sizeof(struct sockaddr_in));
                memcpy(&last_tx, &tx, sizeof(cbit_result_msg));
            }
        }
    }
}
