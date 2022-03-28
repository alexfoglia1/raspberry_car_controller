#ifndef UTILS_H
#define UTILS_H

#include <opencv2/opencv.hpp>
#include <list>

double micros_since_epoch();
double toDegrees(double angle_rad);
double toRadians(double angle_deg);
double normalizeAngle(double angle_rad);
void   filledRoundedRectangle(cv::Mat& src,          //Image where rect is drawn
                      cv::Point topLeft,             //top left corner
                      cv::Size rectSz,               //rectangle size
                      const cv::Scalar fillColor,    //fill color
                      const int lineType,            //type of line
                      const int delta,               //angle between points on the ellipse
                      const float cornerCurvatureRatio); //curvature of the corner

#endif // UTILS_H
