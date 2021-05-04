#ifndef HDR_H
#define HDR_H

#include "core.h"

void coefVar(cv::Mat img, cv::Mat &img_cv, int mask_size = CV_SIZE, bool gauss = true,
             float sigma = SIGMA);

void logTransform(cv::Mat img, cv::Mat &img_log);

#endif
