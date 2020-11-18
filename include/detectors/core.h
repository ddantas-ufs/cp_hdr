#ifndef CORE_H
#define CORE_H

#include <bits/stdc++.h>

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#define DOG_SCL_ROWS 4
#define DOG_SCL_COLS 5
#define CONTRAST_TH 0.03 //prybil set to 8
#define CURV_TH 5
#define GAUSS_SIZE 9
#define SIGMA_X 0 //fix value (ex. 1.0) to keep a standard
#define SIGMA_Y 0 //fix value (ex. 1.0) to keep a standard
#define MAXSUP_SIZE 21
#define SOBEL_SIZE 7
#define K 0.04
#define MIN_QUALITY 0.05
#define MAX_KP 500
#define CV_SIZE 3 //para o harris pode ser 5

struct KeyPoints
{
    int y; //change to float soon
	int x; //change to float soon
	float resp;
	int scale;
	int level;
};

#endif