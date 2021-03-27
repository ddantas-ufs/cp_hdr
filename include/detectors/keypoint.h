#ifndef KEYPOINT_H
#define KEYPOINT_H

#include "core.h"

std::string keypointToString( KeyPoints kp );

bool outOfBounds(int i, int j, cv::Size size_img);

void plotKeyPoints(cv::Mat img, std::vector<KeyPoints> kp, std::string out_path);

void saveKeypoints(std::vector<KeyPoints> kp, std::string out_path, int max_kp = MAX_KP);

std::vector<KeyPoints> loadKeypoints(std::string arqPath);

#endif
