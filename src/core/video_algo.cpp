#include "video_algo.h"

void clahe(cv::Mat* image)
{
    cv::Mat bChannel(image->size().height, image->size().width, CV_8UC1);
    cv::Mat gChannel(image->size().height, image->size().width, CV_8UC1);
    cv::Mat rChannel(image->size().height, image->size().width, CV_8UC1);

    cv::Mat eq_bChannel(image->size().height, image->size().width, CV_8UC1);
    cv::Mat eq_gChannel(image->size().height, image->size().width, CV_8UC1);
    cv::Mat eq_rChannel(image->size().height, image->size().width, CV_8UC1);

    uchar* bData = bChannel.data;
    uchar* gData = gChannel.data;
    uchar* rData = rChannel.data;
    uchar* pData = image->data;
    while (pData < image->dataend - 3)
    {
        *bData = pData[0];
        *gData = pData[1];
        *rData = pData[2];

        bData += 1;
        gData += 1;
        rData += 1;
        pData += 3;
    }

    auto clahe = cv::createCLAHE(2);
    clahe->apply(bChannel, eq_bChannel);
    clahe->apply(gChannel, eq_gChannel);
    clahe->apply(rChannel, eq_rChannel);

    bData = eq_bChannel.data;
    gData = eq_gChannel.data;
    rData = eq_rChannel.data;
    pData = image->data;
    while (pData < image->dataend - 3)
    {
        pData[0] = *bData;
        pData[1] = *gData;
        pData[2] = *rData;

        bData += 1;
        gData += 1;
        rData += 1;
        pData += 3;
    }
}


void polarity(cv::Mat* image)
{
    uchar* pData = image->data;
    while (pData < image->dataend)
    {
        *pData = ~*pData;

        pData++;
    }
}

void channel_filter(cv::Mat* image, int channel_index)
{
    uchar* pData = image->data;

    while (pData < image->dataend - 3)
    {
        pData[channel_index] = 0x00;

        pData += 3;
    }
}
