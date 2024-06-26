#ifndef SIFT_H
#define SIFT_H

#include "../detectors/core.h"
#include "../detectors/aux_func.h"
#include "../detectors/keypoint.h"

void siftDescriptor( std::vector<KeyPoints> &kp, cv::Mat &img, cv::Mat &imgGray,
                     int mGauss = SIFT_DESC_ORIENT_WINDOW,
                     float sigma = SIFT_DESC_ORIENT_SIGMA );

void runSift( cv::Mat img, std::vector<KeyPoints> &kpList, int kpMax, cv::Mat roi, 
              bool isHDR = USE_CV_FILTER_FALSE );

void runSift( cv::Mat img, std::vector<KeyPoints> &kpList, int kpMax,
              bool isHDR = USE_CV_FILTER_FALSE );

void runSift( cv::Mat img, std::vector< std::vector<KeyPoints> > &kpList, int kpMax,
              std::vector<cv::Mat> lRoi, bool isHDR = USE_CV_FILTER_FALSE );

//void runSift( cv::Mat img, std::vector<KeyPoints> &kpList, int kpMax, 
//              std::vector<cv::Mat> lRoi );

#endif