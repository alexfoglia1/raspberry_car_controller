#include "utils.h"
#include <math.h>
#include <sys/time.h>


double micros_since_epoch()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((double)tv.tv_sec*1e6 + (double)tv.tv_usec);
}

double toDegrees(double angle_deg)
{
    return angle_deg * 180.0 / M_PI;
}

double toRadians(double angle_rad)
{
    return angle_rad * M_PI / 180.0;
}

double normalizeAngle(double angle_rad)
{
    double angle_deg_180 = -1 * toDegrees(atan2(sin(angle_rad), cos(angle_rad)));
    return angle_deg_180 >= 0 ? angle_deg_180 : 360.0 - abs(angle_deg_180);
}


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

float avg(std::list<float> list)
{
    float sum = 0.f;
    for(auto& val : list)
    {
        sum += val;
    }

    return sum / (float)list.size();
}

float linearSpeed(float voltage_out)
{
    return 0.000737 + 0.084763 * voltage_out;
}
