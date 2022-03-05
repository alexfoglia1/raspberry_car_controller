#include "tracker.h"

Tracker::Tracker()
{
    rx = 0;
}

uchar corr_at(cv::Mat im1, cv::Mat im2, int i, int j, uchar mean1, uchar mean2)
{
    float numerator = 0;
    for (int m = 0; m < im1.cols - i; m++)
    {
        for (int n = 0; n < im1.rows - j; n++)
        {
            int _i = m + i;
            int _j = n + j;
            numerator += (im1.data[_i * im1.rows + _j] - mean1) * (im2.data[m + im2.rows + n] - mean2);
        }
    }

    float denominator_1 = 0;
    for (int m = 0; m < im1.cols - i; m++)
    {
        for (int n = 0; n < im1.rows - j; n++)
        {
            int _i = m + i;
            int _j = n + j;
            denominator_1 += pow(im1.data[_i * im1.rows + _j] - mean1, 2);
        }
    }

    float denominator_2 = 0;
    for (int m = 0; m < im2.cols - i; m++)
    {
        for (int n = 0; n < im2.rows - j; n++)
        {
            int _i = m + i;
            int _j = n + j;
            denominator_2 += pow(im2.data[_i * im1.rows + _j] - mean1, 2);
        }
    }

    float denominator = denominator_1 * denominator_2;
    denominator = sqrt(denominator);

    float res = numerator / denominator;

    return 0xFF * res;

}

cv::Mat Tracker::correlate(cv::Mat image1, cv::Mat image2, cv::Rect region)
{
    cv::Mat r_im1 = image1(region);
    cv::Mat r_im2 = image2(region);
    int datalen = region.area();

    int sum_1 = 0;
    int sum_2 = 0;

    uchar* p1 = r_im1.data;
    uchar* p2 = r_im2.data;
    while (p1 < r_im1.dataend - 1 && p2 < r_im2.dataend -1)
    {
        sum_1 += *p1;
        sum_2 += *p2;

        ++p1;
        ++p2;
    }

    uchar mean_1 = float(sum_1) / float(datalen);
    uchar mean_2 = float(sum_2) / float(datalen);
    cv::Mat result(region.height, region.width, CV_8U);

    for (int i = 0; i < region.height; i++)
    {
        for (int j = 0; j < region.width; j++)
        {
            result.data[i * region.width + j] = corr_at(r_im1, r_im2, i, j, mean_1, mean_2);
        }
    }

    return result;
}

void Tracker::on_camera_image(cv::Mat frame_from_camera)
{
    cv::Mat copy = frame_from_camera;
    cv::Mat* image = new cv::Mat(copy.size(), copy.type());
    if (rx == 0)
    {
        memcpy(image->data, copy.data, copy.dataend - copy.data);
        old = *image;
        rx++;
    }
    else if (rx == 1)
    {
        memcpy(image->data, copy.data, copy.dataend - copy.data);
        act = *image;
        rx++;
    }
    else
    {
        old = act;
        act = *image;

        cv::Mat dest_old, dest_act;
        cv::Rect region(old.cols / 2 - 10, old.rows / 2 - 10, 20, 20);
        cv::cvtColor(old, dest_old, cv::COLOR_BGR2GRAY);
        cv::cvtColor(act, dest_act, cv::COLOR_BGR2GRAY);
        cv::Mat correlation = correlate(dest_old, dest_act, region);
        uchar* pToCorr = correlation.data;
        for(int i = 0; i < 20; i++)
        {
            for(int j = 0; j < 20; j++)
            {
                printf("%.2f ",*(float*) pToCorr);

                pToCorr += 4;
            }
        }
    }

    delete image;




}
