#include "video_renderer.h"
#include "defs.h"

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

PFNGLWINDOWPOS2IPROC glWindowPos2i;

#define ACC_POS 13, SCREEN_ROWS - 13
#define GYRO_POS 13, SCREEN_ROWS - 26
#define MAGN_POS 13, SCREEN_ROWS - 39
#define SPEED_POS 5 * SCREEN_COLS/7, SCREEN_ROWS - 13
#define ATT_POS 13, 13
#define RAD_POS 5 * SCREEN_COLS/7, 13
#define VIDEO_POS SCREEN_COLS/4, SCREEN_ROWS/6

char acc_display[256];
char gyro_display[256];
char magn_display[256];
char speed_display[256];
char att_display[256];
char rad_display[256];

int imu_socket, speed_socket, attitude_socket, radiation_socket, image_socket;
struct sockaddr_in imu_saddr, speed_saddr, attitude_saddr, radiation_saddr, image_saddr;

uint8_t to_renderize[SCREEN_ROWS][SCREEN_COLS][3];

void print_bitmap_string(void* font, char* s)
{
   if (s && strlen(s))
   {
      while (*s)
      {
         glutBitmapCharacter(font, *s);
         s++;
      }
   }
}


void init_localsock(int* localsocket, struct sockaddr_in* localaddr, int port)
{
    *localsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(*localsocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    memset(localaddr, 0x00, sizeof(struct sockaddr_in));
    localaddr->sin_family = AF_INET;
    localaddr->sin_port = htons(port);
    localaddr->sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(*localsocket, (struct sockaddr*)localaddr, sizeof(struct sockaddr));
}

void render_window()
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glWindowPos2i(VIDEO_POS);
    glDrawPixels(SCREEN_COLS, SCREEN_ROWS, GL_RGB, GL_UNSIGNED_BYTE, (void *)to_renderize);

    glWindowPos2i(ACC_POS);
    print_bitmap_string(GLUT_BITMAP_8_BY_13, acc_display);
    glWindowPos2i(GYRO_POS);
    print_bitmap_string(GLUT_BITMAP_8_BY_13, gyro_display);
    glWindowPos2i(MAGN_POS);
    print_bitmap_string(GLUT_BITMAP_8_BY_13, magn_display);

    glWindowPos2i(ATT_POS);
    print_bitmap_string(GLUT_BITMAP_8_BY_13, att_display);

    glWindowPos2i(SPEED_POS);
    print_bitmap_string(GLUT_BITMAP_8_BY_13, speed_display);

    glWindowPos2i(RAD_POS);
    print_bitmap_string(GLUT_BITMAP_8_BY_13, rad_display);

    glutSwapBuffers();
}


void imu_task()
{
    imu_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(imu_socket, &recv, sizeof(imu_msg), 0, (struct sockaddr*)&imu_saddr, &len);
    if(bytes_recv > 0)
    {
        update_imu(recv);
    }
}

void image_task()
{
    image_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(image_socket, &recv, sizeof(image_msg), 0, (struct sockaddr*)&image_saddr, &len);
    if(bytes_recv > 0)
    {
        update_image(recv);
    }
}

void speed_task()
{
    speed_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(speed_socket, &recv, sizeof(speed_msg), 0, (struct sockaddr*)&speed_saddr, &len);
    if(bytes_recv > 0)
    {
        update_speed(recv);
    }
}

void attitude_task()
{
    attitude_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(attitude_socket, &recv, sizeof(attitude_msg), 0, (struct sockaddr*)&attitude_saddr, &len);
    if(bytes_recv > 0)
    {
        update_attitude(recv);
    }
}

void radiation_task()
{
    radiation_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(radiation_socket, &recv, sizeof(radiation_msg), 0, (struct sockaddr*)&radiation_saddr, &len);
    if(bytes_recv > 0)
    {
        update_radiation(recv);
    }
}

void print_header(msg_header header)
{
    printf("msg_id: %d\n", header.msg_id);
    printf("sse: %f s\n", (double)header.msg_timestamp.tv_sec + 1e-6 * (double)header.msg_timestamp.tv_usec);
}

void update_image(image_msg image)
{
    uint32_t row = image.row;
    uint32_t col = image.col;
    for(uint32_t i = 0; i < BLOCK_ROWS; i++)
    {
        for(uint32_t j = 0; j < BLOCK_COLS; j++)
        {
            to_renderize[row + i][col + j][0] = image.data[i][j][0];
            to_renderize[row + i][col + j][1] = image.data[i][j][2];
            to_renderize[row + i][col + j][2] = image.data[i][j][2];
        }
    }
}

void update_imu(imu_msg imu)
{
    sprintf(acc_display, "Acc : %f %f %f", imu.ax, imu.ay, imu.az);
    sprintf(gyro_display, "Gyro: %f %f %f", imu.gyrox, imu.gyroy, imu.gyroz);
    sprintf(magn_display, "Magn: %f %f %f", imu.magnx, imu.magny, imu.magnz);
}

void update_speed(speed_msg speed)
{
    sprintf(speed_display, "Speed: %f %f %f", speed.vx, speed.vy, speed.vz);
}

void update_attitude(attitude_msg attitude)
{
    sprintf(att_display, "Roll: %f Pitch: %f Yaw: %f", attitude.roll, attitude.pitch, attitude.yaw);
}

void update_radiation(radiation_msg radiation)
{
    sprintf(rad_display, "CPM: %f uSv/h: %f", radiation.CPM, radiation.uSv_h);
}

void main_loop()
{
    imu_task();
    speed_task();
    attitude_task();
    radiation_task();
    image_task();
    render_window();

    usleep(1e4);
}

void init_window(int argc, char** argv)
{
    init_localsock(&imu_socket, &imu_saddr, IMUPORT);
    init_localsock(&speed_socket, &speed_saddr, VELPORT);
    init_localsock(&attitude_socket, &attitude_saddr, ATTPORT);
    init_localsock(&radiation_socket, &radiation_saddr, RADPORT);
    init_localsock(&image_socket, &image_saddr, RENPORT);

    sprintf(acc_display, "Acc : %f %f %f", 0., 0., 0.);
    sprintf(gyro_display, "Gyro: %f %f %f", 0., 0., 0.);
    sprintf(magn_display, "Magn: %f %f %f", 0., 0., 0.);
    sprintf(speed_display, "Speed: %f %f %f", 0., 0., 0.);
    sprintf(att_display, "Roll: %f Pitch: %f Yaw: %f", 0., 0., 0.);
    sprintf(rad_display, "CPM: %f uSv/h: %f", 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT);

    glutInit(&argc, argv); //passo a opengl eventuali argomenti da tastiera
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); //modo standard per inizializzare un display mode che va bene quasi sempre

    glutInitWindowSize(SCREEN_COLS, SCREEN_ROWS); //creo una finestra SCREEN_COLS * SCREEN_ROWS
    glutInitWindowPosition(0, 0); //piazzo la finestra in alto a sx
    glutCreateWindow(PROJNAME); //visualizza la finestra con un titolo
    glutDisplayFunc(render_window); //assegno renderChip8 alla displayFunc ossia la funzione che viene chiamata per renderizzare la memorya di chip8 (credo che questa debba essere eseguita ogni volta che drawFlag = true)
    glutIdleFunc(main_loop);

    glWindowPos2i = (PFNGLWINDOWPOS2IPROC) glutGetProcAddress("glWindowPos2i");

}

void show_window()
{
    glutMainLoop();
}
