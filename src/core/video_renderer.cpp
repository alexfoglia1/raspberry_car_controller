#include "defs.h"
#include "video_renderer.h"
#include "data_transceiver.h"
#include "keymap.h"

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include "logger.h"
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn/dnn.hpp>

#define ACC_POS 13, SCREEN_ROWS - 13
#define GYRO_POS 13, SCREEN_ROWS - 26
#define MAGN_POS 13, SCREEN_ROWS - 39
#define SPEED_POS 5 * SCREEN_COLS/7, SCREEN_ROWS - 13
#define ATT_POS 13, 13
#define RAD_POS 5 * SCREEN_COLS/7, 13
#define VIDEO_POS SCREEN_COLS/4, SCREEN_ROWS/6

#ifdef CV_CXX11
template <typename T>
class QueueFPS : public std::queue<T>
{
public:
    QueueFPS() : counter(0) {}
    void push(const T& entry)
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::queue<T>::push(entry);
        counter += 1;
        if (counter == 1)
        {
            // Start counting from a second frame (warmup).
            tm.reset();
            tm.start();
        }
    }
    T get()
    {
        std::lock_guard<std::mutex> lock(mutex);
        T entry = this->front();
        this->pop();
        return entry;
    }
    float getFPS()
    {
        tm.stop();
        double fps = counter / tm.getTimeSec();
        tm.start();
        return static_cast<float>(fps);
    }
    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        while (!this->empty())
            this->pop();
    }
    unsigned int counter;
private:
    cv::TickMeter tm;
    std::mutex mutex;
};
#endif  // CV_CXX11

float act_yaw_deg = 0.0;
float act_pitch_deg = 0.0;
float act_roll_deg = 0.0;

const int H_FOV_DEG = 53;
const int los_ray = 50;

const float confThreshold = 0.25f;
const float nmsThreshold = 0.4f;
std::vector<std::string> classes;

char acc_display[256];
char gyro_display[256];
char magn_display[256];
char speed_display[256];
char roll_display[256];
char pitch_display[256];
char yaw_display[256];
char rad_display[256];
char cmd_display[256];
char throttle_display[256];

QueueFPS<cv::Mat> processedFramesQueue;
QueueFPS<std::vector<cv::Mat>> predictionsQueue;
std::queue<cv::AsyncArray> futureOutputs;
QueueFPS<cv::Mat> framesQueue;

int imu_socket, speed_socket, attitude_socket, radiation_socket, throttle_socket, image_socket;
struct sockaddr_in imu_saddr, speed_saddr, attitude_saddr, radiation_saddr, throttle_saddr, image_saddr;
cv::Mat* imagewindow;

void drawPred(int classId, float conf, int left, int top, int right, int bottom, cv::Mat& frame)
{
    rectangle(frame, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(0, 255, 0));
    std::string label = cv::format("%.2f", conf);
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ": " + label;
    }
    int baseLine;
    cv::Size labelSize = getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    top = cv::max(top, labelSize.height);
    rectangle(frame, cv::Point(left, top - labelSize.height),
              cv::Point(left + labelSize.width, top + baseLine), cv::Scalar::all(255), cv::FILLED);
    putText(frame, label, cv::Point(left, top), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar());
}


void preprocess(const cv::Mat& frame, cv::dnn::Net& net, cv::Size inpSize, float scale,
                       const cv::Scalar& mean, bool swapRB)
{
    static cv::Mat blob;
    // Create a 4D blob from a frame.
    if (inpSize.width <= 0) inpSize.width = frame.cols;
    if (inpSize.height <= 0) inpSize.height = frame.rows;
    cv::dnn::blobFromImage(frame, blob, 1.0, inpSize, cv::Scalar(), swapRB, false, CV_8U);
    // Run a model.
    net.setInput(blob, "", scale, mean);
    if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
    {
        resize(frame, frame, inpSize);
        cv::Mat imInfo = (cv::Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
        net.setInput(imInfo, "im_info");
    }
}


void postprocess(cv::Mat& frame, const std::vector<cv::Mat>& outs, cv::dnn::Net& net, int backend)
{
    static std::vector<int> outLayers = net.getUnconnectedOutLayers();
    static std::string outLayerType = net.getLayer(outLayers[0])->type;
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    if (outLayerType == "DetectionOutput")
    {
        CV_Assert(outs.size() > 0);
        for (size_t k = 0; k < outs.size(); k++)
        {
            float* data = (float*)outs[k].data;
            for (size_t i = 0; i < outs[k].total(); i += 7)
            {
                float confidence = data[i + 2];
                if (confidence > confThreshold)
                {
                    int left   = (int)data[i + 3];
                    int top    = (int)data[i + 4];
                    int right  = (int)data[i + 5];
                    int bottom = (int)data[i + 6];
                    int width  = right - left + 1;
                    int height = bottom - top + 1;
                    if (width <= 2 || height <= 2)
                    {
                        left   = (int)(data[i + 3] * frame.cols);
                        top    = (int)(data[i + 4] * frame.rows);
                        right  = (int)(data[i + 5] * frame.cols);
                        bottom = (int)(data[i + 6] * frame.rows);
                        width  = right - left + 1;
                        height = bottom - top + 1;
                    }
                    classIds.push_back((int)(data[i + 1]) - 1);  // Skip 0th background class id.
                    boxes.push_back(cv::Rect(left, top, width, height));
                    confidences.push_back(confidence);
                }
            }
        }
    }
    else if (outLayerType == "Region")
    {
        for (size_t i = 0; i < outs.size(); ++i)
        {
            // Network produces output blob with a shape NxC where N is a number of
            // detected objects and C is a number of classes + 4 where the first 4
            // numbers are [center_x, center_y, width, height]
            float* data = (float*)outs[i].data;
            for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
            {
                cv::Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
                cv::Point classIdPoint;
                double confidence;
                minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
                if (confidence > confThreshold)
                {
                    int centerX = (int)(data[0] * frame.cols);
                    int centerY = (int)(data[1] * frame.rows);
                    int width = (int)(data[2] * frame.cols);
                    int height = (int)(data[3] * frame.rows);
                    int left = centerX - width / 2;
                    int top = centerY - height / 2;
                    classIds.push_back(classIdPoint.x);
                    confidences.push_back((float)confidence);
                    boxes.push_back(cv::Rect(left, top, width, height));
                }
            }
        }
    }
    else
        CV_Error(cv::Error::StsNotImplemented, "Unknown output layer type: " + outLayerType);
    // NMS is used inside Region layer only on DNN_BACKEND_OPENCV for another backends we need NMS in sample
    // or NMS is required if number of outputs > 1
    if (outLayers.size() > 1 || (outLayerType == "Region" && backend != cv::dnn::DNN_BACKEND_OPENCV))
    {
        std::map<int, std::vector<size_t> > class2indices;
        for (size_t i = 0; i < classIds.size(); i++)
        {
            if (confidences[i] >= confThreshold)
            {
                class2indices[classIds[i]].push_back(i);
            }
        }
        std::vector<cv::Rect> nmsBoxes;
        std::vector<float> nmsConfidences;
        std::vector<int> nmsClassIds;
        for (std::map<int, std::vector<size_t> >::iterator it = class2indices.begin(); it != class2indices.end(); ++it)
        {
            std::vector<cv::Rect> localBoxes;
            std::vector<float> localConfidences;
            std::vector<size_t> classIndices = it->second;
            for (size_t i = 0; i < classIndices.size(); i++)
            {
                localBoxes.push_back(boxes[classIndices[i]]);
                localConfidences.push_back(confidences[classIndices[i]]);
            }
            std::vector<int> nmsIndices;
            cv::dnn::NMSBoxes(localBoxes, localConfidences, confThreshold, nmsThreshold, nmsIndices);
            for (size_t i = 0; i < nmsIndices.size(); i++)
            {
                size_t idx = nmsIndices[i];
                nmsBoxes.push_back(localBoxes[idx]);
                nmsConfidences.push_back(localConfidences[idx]);
                nmsClassIds.push_back(it->first);
            }
        }
        boxes = nmsBoxes;
        classIds = nmsClassIds;
        confidences = nmsConfidences;
    }
    for (size_t idx = 0; idx < boxes.size(); ++idx)
    {
        cv::Rect box = boxes[idx];
        drawPred(classIds[idx], confidences[idx], box.x, box.y,
                 box.x + box.width, box.y + box.height, frame);
    }
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

void init_localsock(int* localsocket, struct sockaddr_in* localaddr, int port)
{
    *localsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 1;
    setsockopt(*localsocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    memset(localaddr, 0x00, sizeof(struct sockaddr_in));
    localaddr->sin_family = AF_INET;
    localaddr->sin_port = htons(port);
    localaddr->sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(*localsocket, (struct sockaddr*)localaddr, sizeof(struct sockaddr));
}

void update_image(image_msg image, cv::dnn::Net vNet)
{
    std::vector<char> data(image.data, image.data + image.len);

    *imagewindow = cv::imdecode(cv::Mat(data), 1);
    cv::resize(*imagewindow, *imagewindow, cv::Size(1200,650));
#ifdef MACHINE_LEARNING

    cv::Mat frame = cv::imdecode(cv::Mat(data), 1);
    preprocess(frame, vNet, cv::Size(608, 608), 1, cv::Scalar(0.5), false);
    processedFramesQueue.push(frame);

    std::vector<cv::Mat> outs;
    vNet.forward(outs, vNet.getUnconnectedOutLayersNames());

    while (!futureOutputs.empty() &&
           futureOutputs.front().wait_for(std::chrono::seconds(1)))
    {
        cv::AsyncArray async_out = futureOutputs.front();
        futureOutputs.pop();
        cv::Mat out;
        async_out.get(out);
        predictionsQueue.push({out});
    }

    if(predictionsQueue.empty()) return;

    std::vector<cv::Mat> out_pred = predictionsQueue.get();
    cv::Mat frame_out = processedFramesQueue.get();
    postprocess(frame_out, out_pred, vNet, cv::dnn::DNN_BACKEND_OPENCV);
    if (predictionsQueue.counter > 1)
    {
        std::string label = cv::format("Camera: %.2f FPS", framesQueue.getFPS());
        putText(frame_out, label, cv::Point(0, 15), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0));
        label = cv::format("Network: %.2f FPS", predictionsQueue.getFPS());
        putText(frame_out, label, cv::Point(0, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0));
        label = cv::format("Skipped frames: %d", framesQueue.counter - predictionsQueue.counter);
        putText(frame_out, label, cv::Point(0, 45), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0));

    }

#endif

}



void update_imu(imu_msg imu)
{

    sprintf(acc_display,  "Acc. : %.1f %.1f %.1f", imu.ax, imu.ay, imu.az);
    sprintf(gyro_display, "Gyro.: %.1f %.1f %.1f", imu.gyrox, imu.gyroy, imu.gyroz);
    sprintf(magn_display, "Magn.: %.1f %.1f %.1f", imu.magnx, imu.magny, imu.magnz);
}

void update_speed(speed_msg speed)
{
    sprintf(speed_display, "Speed: %.1f %.1f %.1f", speed.vx, speed.vy, speed.vz);
}


void update_attitude(attitude_msg attitude)
{

    sprintf(roll_display, "Roll: %.1f", normalizeAngle(toRadians(attitude.roll)));
    sprintf(pitch_display, "Pitch: %.1f", normalizeAngle(toRadians(attitude.pitch)));
    sprintf(yaw_display, "Yaw: %.1f", normalizeAngle(toRadians(attitude.yaw)));

    act_yaw_deg = -normalizeAngle(toRadians(attitude.yaw));
    act_pitch_deg = -normalizeAngle(toRadians(attitude.pitch));
    act_roll_deg = -normalizeAngle(toRadians(attitude.roll));
}

void update_radiation(radiation_msg radiation)
{
    sprintf(rad_display, "CPM: %.1f uSv/h: %.1f", radiation.CPM, radiation.uSv_h);
}

void update_throttle(uint8_t th_state)
{
    char th_progress[100];
    float perc = ((float)th_state / 255.0) * 100.0;

    for(uint8_t i = 0; i < 100; i++)
    {
        th_progress[i] = (i < perc) ? '*' : ' ';
    }

    sprintf(throttle_display, "[%s]", th_progress);
}

void render_window()
{
    cv::Size size = imagewindow->size();

    std::string text_acc(acc_display);
    std::string text_gyro(gyro_display);
    std::string text_magn(magn_display);
    std::string text_speed(speed_display);
    std::string text_roll(roll_display);
    std::string text_pitch(pitch_display);
    std::string text_yaw(yaw_display);
    std::string text_radiation(rad_display);
    std::string text_command(cmd_display);
    std::string text_throttle(throttle_display);

    double left_bound_deg = normalizeAngle(toRadians((act_yaw_deg - H_FOV_DEG / 2)));
    double right_bound_deg = normalizeAngle(toRadians((act_yaw_deg + H_FOV_DEG / 2)));
    double pitch90 = -asin(sin((atan2(sin(act_pitch_deg * 3.14/180.0), cos(act_pitch_deg * 3.14/180.0)) * 180.0/3.14) * 3.14/180.0)) * 180.0/3.14;
    double elevPercentage = pitch90 / 90;

    cv::Point2d losCenter(size.width - 60U, size.height/2);
    cv::Point2d losLeft(losCenter.x + los_ray * cos(toRadians(left_bound_deg - 90)), losCenter.y + los_ray * sin(toRadians(left_bound_deg - 90)));
    cv::Point2d losRight(losCenter.x + los_ray * cos(toRadians(right_bound_deg - 90)), losCenter.y + los_ray * sin(toRadians(right_bound_deg - 90)));
    cv::Point2d losElev(losCenter.x, losCenter.y + elevPercentage * los_ray);
    cv::Point2d losRollLeft(losCenter.x + los_ray * cos(toRadians(act_roll_deg)), losCenter.y + los_ray * sin(toRadians(act_roll_deg)));
    cv::Point2d losRollRight(losCenter.x - los_ray * cos(toRadians(act_roll_deg)), losCenter.y - los_ray * sin(toRadians(act_roll_deg)));
    cv::ellipse(*imagewindow, losCenter, cv::Size(los_ray, los_ray), 15, 0, 360, cv::Scalar(0, 255, 0), 1);
    cv::drawMarker(*imagewindow,losElev, cv::Scalar(0, 255, 0));
    cv::line(*imagewindow, losCenter, losLeft, cv::Scalar(0, 255, 0), 1);
    cv::line(*imagewindow, losCenter, losRight, cv::Scalar(0, 255, 0), 1);
    cv::line(*imagewindow, losRollLeft, losRollRight, cv::Scalar(0, 255, 0), 1);
    cv::putText(*imagewindow, text_acc, cv::Point2d(2U, 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_gyro, cv::Point2d(2U, 40U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_magn, cv::Point2d(2U, 60U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_speed, cv::Point2d(2U, 80U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_roll, cv::Point2d(2U, size.height/2 - 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_pitch, cv::Point2d(2U, size.height/2), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_yaw, cv::Point2d(2U, size.height/2 + 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_radiation, cv::Point2d(2U, size.height - 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_command, cv::Point2d(size.width - 120U, 20U), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
    cv::putText(*imagewindow, text_throttle, cv::Point2d(size.width/2, size.height - 20U), cv::FONT_HERSHEY_SIMPLEX, 0.175, cv::Scalar(255,0,0), 1, cv::LINE_AA);

    cv::imshow(PROJNAME, *imagewindow);
}

void cmd_out_task(const char* board_address)
{
    int key = cv::waitKey(10);
    if(key != -1)
    {
        bool error;
        command_msg cmd_out = getCommandOfKey(key, &error);
        writelog("Button press event");
        sprintf(cmd_display, "OUT: %s", getNameOfKey(key));
        if(!error)
        {
            writelog("Send command to board");
            send_data_to_board(reinterpret_cast<char*>(&cmd_out), board_address);
        }
    }
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

void image_task(cv::dnn::Net net)
{
    image_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(image_socket, &recv, sizeof(image_msg), 0, (struct sockaddr*)&image_saddr, &len);
    if(bytes_recv > 0)
    {
        memset(imagewindow->data, 0x00, imagewindow->dataend - imagewindow->data);
        update_image(recv, net);
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
    char crecv[32];

    socklen_t len;
    ssize_t bytes_recv = recvfrom(attitude_socket, &recv, sizeof(attitude_msg), 0, (struct sockaddr*)&attitude_saddr, &len);
    if(bytes_recv > 0)
    {
        memcpy(crecv, &recv, 32);
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

void throttle_task()
{
    throttle_msg recv;
    socklen_t len;
    ssize_t bytes_recv = recvfrom(throttle_socket, &recv, sizeof(throttle_msg), 0, (struct sockaddr*)&throttle_saddr, &len);
    if(bytes_recv > 0)
    {
        update_throttle(recv.throttle_state);
    }
}

void main_loop(const char* board_address, cv::dnn::Net net)
{
    while(true)
    {
        imu_task();
        speed_task();
        attitude_task();

        radiation_task();

        throttle_task();

        image_task(net);

        render_window();

        cmd_out_task(board_address);
    }
}

void init_window()
{

    init_localsock(&imu_socket, &imu_saddr, IMUPORT);
    init_localsock(&speed_socket, &speed_saddr, VELPORT);
    init_localsock(&attitude_socket, &attitude_saddr, ATTPORT);
    init_localsock(&radiation_socket, &radiation_saddr, RADPORT);
    init_localsock(&throttle_socket, &throttle_saddr, THRPORT);
    init_localsock(&image_socket, &image_saddr, RENPORT);

    sprintf(acc_display,   "Acc. : %.1f %.1f %.1f", 0., 0., 0.);
    sprintf(gyro_display,  "Gyro.: %.1f %.1f %.1f", 0., 0., 0.);
    sprintf(magn_display,  "Magn.: %.1f %.1f %.1f", 0., 0., 0.);
    sprintf(speed_display, "Speed: %.1f %.1f %.1f", 0., 0., 0.);
    sprintf(roll_display, "Roll: %.1f", 0.);
    sprintf(pitch_display, "Pitch: %.1f", 0.);
    sprintf(yaw_display, "Yaw: %.1f", 0.);
    sprintf(rad_display,   "CPM: %.1f uSv/h: %.1f", 0., 0.);
    sprintf(cmd_display, "OUT: NONE");
    sprintf(throttle_display, "[                                                                                                    ]");
    imagewindow = new cv::Mat(600, 600, CV_8UC3, cv::Scalar(0, 0, 0));

    cv::imshow(PROJNAME, *imagewindow);
}

