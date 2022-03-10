#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>

#define MAX_IMAGESIZE 60000
#define IMAGE_ROWS    650
#define IMAGE_COLS    1200
#define PORT          2222

typedef struct
{
    uint16_t len;
    uint8_t data[MAX_IMAGESIZE];
} __attribute__((packed)) image_msg;

void filledRoundedRectangle(cv::Mat& src,            //Image where rect is drawn
                      cv::Point topLeft,             //top left corner
                      cv::Size rectSz,               //rectangle size
                      const cv::Scalar fillColor,    //fill color
                      const int lineType,            //type of line
                      const int delta,               //angle between points on the ellipse
                      const float cornerCurvatureRatio) //curvature of the corner
{
    // corners:
    // p1 - p2
    // |     |
    // p4 - p3
    //
    cv::Point p1 = topLeft;
    cv::Point p2 = cv::Point (p1.x + rectSz.width, p1.y);
    cv::Point p3 = cv::Point (p1.x + rectSz.width, p1.y + rectSz.height);
    cv::Point p4 = cv::Point (p1.x, p1.y + rectSz.height);
    int cornerRadius = static_cast<int>(rectSz.height*cornerCurvatureRatio);
    std::vector<cv::Point> points;
    std::vector<cv::Point> pts;
    // Add arcs points

    cv::Size rad = cv::Size(cornerRadius, cornerRadius);

    // segments:
    //    s2____s3
    // s1          s4
    // |           |
    // s8          s5
    //   s7_____s6
    //
    //Add arc s1 to s2
    cv::ellipse2Poly(p1 + cv::Point(cornerRadius, cornerRadius)  , rad, 180.0, 0, 90, delta , pts);
    points.insert(points.end(), pts.begin(), pts.end());
    pts.clear();
    //Add line s2-s3
    points.push_back(cv::Point (p1.x + cornerRadius, p1.y)); points.push_back(cv::Point (p2.x - cornerRadius, p2.y));

    //Add arc s3 to s4
    cv::ellipse2Poly(p2 + cv::Point(-cornerRadius, cornerRadius) , rad, 270.0, 0, 90, delta, pts);
    points.insert(points.end(), pts.begin(), pts.end());
    pts.clear();
    //Add line s4 to s5
    points.push_back(cv::Point (p2.x, p2.y + cornerRadius)); points.push_back(cv::Point (p3.x, p3.y - cornerRadius));

    //Add arc s5 to s6
    cv::ellipse2Poly(p3 + cv::Point(-cornerRadius, -cornerRadius), rad, 0.0,   0, 90, delta, pts);
    points.insert(points.end(), pts.begin(), pts.end());
    pts.clear();
    //Add line s7 to s8
    points.push_back(cv::Point (p4.x + cornerRadius, p4.y)); points.push_back(cv::Point (p3.x - cornerRadius, p3.y));

    //Add arc s7 to s8
    cv::ellipse2Poly(p4 + cv::Point(cornerRadius, -cornerRadius) , rad, 90.0,  0, 90, delta, pts);
    points.insert(points.end(), pts.begin(), pts.end());
    //Add line s1 to s8
    points.push_back(cv::Point (p1.x, p1.y + cornerRadius)); points.push_back(cv::Point (p4.x, p4.y - cornerRadius));

    //fill polygon
    cv::fillConvexPoly(src, points, fillColor, lineType);
}

int main(int argc, char** argv)
{
    bool moving = false;
    if (argc < 3)
    {
        printf("USAGE: %s true/false [addr]:\n\ttrue: moving\n\tfalse: fixed\n", argv[0]);
    }
    else
    {
        moving = strcmp("true", argv[1]) == 0;
    }
    printf("Moving: %s\n", moving ? "true" : "false");

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in addr;
    memset(&addr, 0x00, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    int width = 50;
    int height = 50;
    int x0 = IMAGE_COLS / 2 - width/2;
    int y0 = IMAGE_ROWS / 2 - height/2 - 50;
    int dt_millis = 100;
    int t_millis = 0;
    int t = t_millis - dt_millis;
    double vel_t = 0.0001225;

    while (true)
    {
        cv::Mat sim_image = cv::Mat(IMAGE_ROWS, IMAGE_COLS, CV_8UC3, cv::Scalar(0,0,0));
        t += dt_millis;
        int _t = moving ? t : 0;
        int y = y0 + 50 + 200 * sin(vel_t * _t);
        int x = x0 + 200 * sin((vel_t*2) * _t);
        int y2 = y;
        int x2 = x0 + 200 * cos((vel_t*2) *_t);
	y = y % IMAGE_ROWS;
	x = x % IMAGE_COLS;
        filledRoundedRectangle(sim_image, cv::Point(x, y), cv::Size(width, height), cv::Scalar(255,255,255), cv::LINE_AA, 1, 0.01);
        filledRoundedRectangle(sim_image, cv::Point(x2, y2), cv::Size(width/2, height/2), cv::Scalar(255,255,255), cv::LINE_AA, 1, 0.1);

        std::vector<int> params;
        std::vector<uint8_t> buffer;

        params.push_back(cv::IMWRITE_JPEG_QUALITY);
        params.push_back(75);
        cv::imencode(".jpg", sim_image, buffer, params);

        image_msg outmsg;
        outmsg.len = static_cast<uint16_t>(buffer.size());

        for (uint16_t i = 0; i < outmsg.len; i++)
        {
            outmsg.data[i] = buffer[i];
        }

        sendto(s, &outmsg, sizeof(image_msg), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
        //perror("sendto");
        usleep(dt_millis * 1000);
    }


}
