#include "video_algo.h"

VideoProcessor::VideoProcessor()
{
    history.available_frames = 0;
    clahe_enabled = false;
    polarity_enabled = false;
    filter_enabled[0] = filter_enabled[1] = filter_enabled[2] = false;
    denoise_enabled = false;
}

void VideoProcessor::do_clahe()
{
    if (history.available_frames == 0) return;
    cv::Mat* image = history.available_frames == 1 ? &history.frame_0 :
                     history.available_frames == 2 ? &history.frame_1 : &history.frame_2;

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


void VideoProcessor::do_polarity()
{
    if (history.available_frames == 0) return;
    cv::Mat* image = history.available_frames == 1 ? &history.frame_0 :
                     history.available_frames == 2 ? &history.frame_1 : &history.frame_2;

    uchar* pData = image->data;
    while (pData < image->dataend)
    {
        *pData = ~*pData;

        pData++;
    }
}

void VideoProcessor::do_channel_filter(int channel_index)
{
    if (history.available_frames == 0) return;
    cv::Mat* image = history.available_frames == 1 ? &history.frame_0 :
                     history.available_frames == 2 ? &history.frame_1 : &history.frame_2;

    uchar* pData = image->data;

    while (pData < image->dataend - 3)
    {
        pData[channel_index] = 0x00;

        pData += 3;
    }
}


void VideoProcessor::feed(cv::Mat image)
{
    switch(history.available_frames)
    {
    case 0:
        history.frame_0 = image.clone();
        history.available_frames = 1;
        break;
    case 1:
        history.frame_1 = image.clone();
        history.available_frames = 2;
        break;
    case 2:
        history.frame_2 = image.clone();
        history.available_frames = 3;
        break;
    default:
        history.frame_0 = history.frame_1;
        history.frame_1 = history.frame_2;
        history.frame_2 = image.clone();

        break;
    }

    if (clahe_enabled)
    {
        do_clahe();
    }

    if (polarity_enabled)
    {
        do_polarity();
    }

    if (filter_enabled[0])
    {
        do_channel_filter(0);
    }

    if (filter_enabled[1])
    {
        do_channel_filter(1);
    }

    if (filter_enabled[2])
    {
        do_channel_filter(2);
    }

    if (denoise_enabled)
    {
        do_denoise();
    }
    else
    {
        emit frame_ready(history.available_frames == 1 ? history.frame_0 :
                         history.available_frames == 2 ? history.frame_1 :
                         history.frame_2);
    }
}

void VideoProcessor::clahe(bool enabled)
{
    clahe_enabled = enabled;
}

void VideoProcessor::polarity(bool enabled)
{
    polarity_enabled = enabled;
}

void VideoProcessor::channel_filter(bool enabled, int channel_index)
{
    filter_enabled[channel_index] = enabled;
}

void VideoProcessor::denoise(bool enabled)
{
    denoise_enabled = enabled;
}

void VideoProcessor::do_denoise()
{
    if (history.available_frames < 3)
    {
        return;
    }

    cv::Mat copy = history.frame_2.clone();
    cv::Mat* image = &copy;

    uchar* p_frame_0_channel_1 = history.frame_0.data + 0;
    uchar* p_frame_0_channel_2 = history.frame_0.data + 1;
    uchar* p_frame_0_channel_3 = history.frame_0.data + 2;

    uchar* p_frame_1_channel_1 = history.frame_1.data + 0;
    uchar* p_frame_1_channel_2 = history.frame_1.data + 1;
    uchar* p_frame_1_channel_3 = history.frame_1.data + 2;

    uchar* p_frame_2_channel_1 = history.frame_1.data + 0;
    uchar* p_frame_2_channel_2 = history.frame_1.data + 1;
    uchar* p_frame_2_channel_3 = history.frame_1.data + 2;

    uchar* p_data = image->data;

    while (p_data < image->dataend - 3)
    {
        int sum_channel_1 = *(p_data + 0) + *(p_frame_2_channel_1) + *(p_frame_1_channel_1) + *(p_frame_0_channel_1);
        int sum_channel_2 = *(p_data + 1) + *(p_frame_2_channel_2) + *(p_frame_1_channel_2) + *(p_frame_0_channel_2);
        int sum_channel_3 = *(p_data + 2) + *(p_frame_2_channel_3) + *(p_frame_1_channel_3) + *(p_frame_0_channel_3);

        p_data[0] = uchar(float(sum_channel_1) / 3.0);
        p_data[1] = uchar(float(sum_channel_2) / 3.0);
        p_data[2] = uchar(float(sum_channel_3) / 3.0);

        p_data += 3;
        p_frame_0_channel_1 += 3;
        p_frame_0_channel_2 += 3;
        p_frame_0_channel_3 += 3;
        p_frame_1_channel_1 += 3;
        p_frame_1_channel_2 += 3;
        p_frame_1_channel_3 += 3;
        p_frame_2_channel_1 += 3;
        p_frame_2_channel_2 += 3;
        p_frame_2_channel_3 += 3;
    }

    emit frame_ready(*image);
}
