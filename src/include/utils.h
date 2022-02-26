#ifndef UTILS_H
#define UTILS_H

#include <opencv2/imgproc.hpp>
#include <list>

double micros_since_epoch();
double toDegrees(double angle_deg);
double toRadians(double angle_rad);
double normalizeAngle(double angle_rad);
void   filledRoundedRectangle(cv::Mat& src,          //Image where rect is drawn
                      cv::Point topLeft,             //top left corner
                      cv::Size rectSz,               //rectangle size
                      const cv::Scalar fillColor,    //fill color
                      const int lineType,            //type of line
                      const int delta,               //angle between points on the ellipse
                      const float cornerCurvatureRatio); //curvature of the corner
float avg(std::list<float> list);
float linearSpeed(float voltage_out);

#endif // UTILS_H
