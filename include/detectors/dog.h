#ifndef DOG_H
#define DOG_H

#include "core.h"
#include "aux_func.h"
#include "keypoint.h"

void dogKp(cv::Mat img, std::vector<KeyPoints> &kp, bool isHDR = USE_CV_FILTER_FALSE, 
           bool refine_px = true, int mgauss = GAUSS_SIZE, int maxsup_size = MAXSUP_SIZE,
           float contrast_th = CONTRAST_TH, float curv_th = CURV_TH, int cv_size = CV_SIZE );

#endif
