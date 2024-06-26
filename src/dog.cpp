#include "../include/detectors/dog.h"
#include "../include/detectors/hdr.h"

/**
 * Computes the partial derivatives in \a y, \a x, and \a s (scale of a pixel in the DoG scale space pyramid)
 * @param dog DoG scale space pyramid
 * @param o octave of the keypoint in \a dog
 * @param s scale of the keypoint in \a o
 * @param y row of the keypoint
 * @param x column of the keypoint
 * @return the vector of partial derivatives for the pixel as a CvMat*
**/
CvMat* deriv3D(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], int o, int s, int y, int x)
{
  CvMat *dI;
  cv::Mat up = dog[o][s + 1];
  cv::Mat middle = dog[o][s];
  cv::Mat down = dog[o][s - 1];
  double dx, dy, ds;

  dx = (middle.at<float>(y, x + 1) - middle.at<float>(y, x - 1)) / 2.0;
  dy = (middle.at<float>(y + 1, x) - middle.at<float>(y - 1, x)) / 2.0;
  ds = (up.at<float>(y, x) - down.at<float>(y, x)) / 2.0;

  dI = cvCreateMat(3, 1, CV_64FC1);
  cvmSet(dI, 0, 0, dx);
  cvmSet(dI, 1, 0, dy);
  cvmSet(dI, 2, 0, ds);

  return dI;
}

/**
 * Computes the 3D Hessian matrix for a keypoint in the DoG scale space pyramid
 * @param dog DoG scale space pyramid
 * @param o octave of the keypoint in \a dog
 * @param s scale of the keypoint in \a o
 * @param y row of the keypoint
 * @param x column of the keypoint
 * @return the Hessian matrix for keypoint I as a CvMat*
**/
CvMat* hessian3D(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], int o, int s, int y,
                 int x)
{
  CvMat *H;
  cv::Mat up = dog[o][s + 1];
  cv::Mat middle = dog[o][s];
  cv::Mat down = dog[o][s - 1];
  double v, dxx, dyy, dss, dxy, dxs, dys;

  v = middle.at<float>(y, x);

  dxx = middle.at<float>(y, x + 1) + middle.at<float>(y, x - 1) - 2 * v;
  dyy = middle.at<float>(y + 1, x) + middle.at<float>(y - 1, x) - 2 * v;
  dss = up.at<float>(y, x) + down.at<float>(y, x) - 2 * v;
  dxy = (middle.at<float>(y + 1, x + 1) - middle.at<float>(y + 1, x - 1) -
         middle.at<float>(y - 1, x + 1) + middle.at<float>(y - 1, x - 1)) / 4.0;
  dxs = (up.at<float>(y, x + 1) - up.at<float>(y, x - 1) - down.at<float>(y, x + 1) +
         down.at<float>(y, x - 1)) / 4.0;
  dys = (up.at<float>(y + 1, x) - up.at<float>(y - 1, x) - down.at<float>(y + 1, x) +
         down.at<float>(y - 1, x)) / 4.0;

  H = cvCreateMat(3, 3, CV_64FC1);
  cvmSet(H, 0, 0, dxx);
  cvmSet(H, 0, 1, dxy);
  cvmSet(H, 0, 2, dxs);
  cvmSet(H, 1, 0, dxy);
  cvmSet(H, 1, 1, dyy);
  cvmSet(H, 1, 2, dys);
  cvmSet(H, 2, 0, dxs);
  cvmSet(H, 2, 1, dys);
  cvmSet(H, 2, 2, dss);

  return H;
}

/**
 * Performs one step of extremum interpolation
 * @param dog DoG scale space pyramid
 * @param o octave of the keypoint in \a dog
 * @param s scale of the keypoint in \a o
 * @param y row of the keypoint
 * @param x column of the keypoint
 * @param inc_s output as interpolated subpixel increment to scale
 * @param inc_y output as interpolated subpixel increment to row
 * @param inc_x output as interpolated subpixel increment to col
**/

void interpStep(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], int o, int s, int y,
                int x, double *inc_s, double *inc_y, double *inc_x)
{
  CvMat *dD, *H, *H_inv, X;
  double inc[3] = {0};

  dD = deriv3D(dog, o, s, y, x);
  H = hessian3D(dog, o, s, y, x);
  H_inv = cvCreateMat(3, 3, CV_64FC1);
  cvInvert(H, H_inv, CV_SVD);
  cvInitMatHeader(&X, 3, 1, CV_64FC1, inc, CV_AUTOSTEP);
  cvGEMM(H_inv, dD, -1, NULL, 0, &X, 0);

  *inc_s = inc[2];
  *inc_y = inc[1];
  *inc_x = inc[0];
}

/**
 * Interpolates a scale-space extremum's location and scale to subpixel accuracy to
 * form an image feature (keypoint)
 * @param dog DoG scale space pyramid
 * @param o octave of the keypoint in \a dog
 * @param s scale of the keypoint in \a o
 * @param y row of the keypoint
 * @param x column of the keypoint
 * @param num_scales total scales per octave
 * @return the keypoint resulting from interpolation of the given parameters or NULL if
 * the given location could not be interpolated
**/
KeyPoints interpExtremum(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], int o, int s,
                         int y, int x, int num_scales)
{
  KeyPoints kp;
  double inc_s, inc_y, inc_x;
  int i = 0;

  // MINIMUM TYPE VALUES TO KEYPOINT
  kp.x = std::numeric_limits<float>::min();
  kp.y = std::numeric_limits<float>::min();
  kp.octave = std::numeric_limits<int>::min();
  kp.scale = std::numeric_limits<int>::min();

  while (i < MAX_INTERP_STEPS)
  {
    interpStep(dog, o, s, y, x, &inc_s, &inc_y, &inc_x);
    if(std::abs(inc_s) < 0.5 && std::abs(inc_y) < 0.5 && std::abs(inc_x) < 0.5)
    {
      break;
    }
    s += cvRound(inc_s);
    y += cvRound(inc_y);
    x += cvRound(inc_x);

    if (s < 1  || s > num_scales  || x < DOG_BORDER  || y < DOG_BORDER  ||
        x >= dog[o][0].rows - DOG_BORDER  || y >= dog[o][0].cols - DOG_BORDER)
    {
      return kp;
    }
    i++;
  }

  if (i >= MAX_INTERP_STEPS)
  {
    return kp;
  }

  kp.y = y + inc_y;
  kp.x = x + inc_x;
  kp.octave = o;
  kp.scale = s;

  return kp;
}

/**
 * Is the local extrema detection step. It detects the local maxima and minima points
 * in DoG images
 * @param dog DoG scale space pyramid
 * @param kp array where keypoints are stored
 * @param maxsup_size maximum number of checks when locating maxima/minima in DoG images
**/
void dogMaxSup(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], std::vector<KeyPoints> &kp,
               float maxsup_size, float curv_th, bool refine_px)
{
  int maxsup_rad = maxsup_size/2;

  for (int o = 0; o < NUM_OCTAVES; o++)
  {
    for (int s = 1; s < NUM_SCALES - 1; s++)
    {
      cv::Mat middle = dog[o][s];
      cv::Mat down = dog[o][s - 1];
      cv::Mat up = dog[o][s + 1];

      for (int y = maxsup_rad; y < middle.rows - maxsup_rad; y++)
      {
        for (int x = maxsup_rad; x < middle.cols - maxsup_rad; x++)
        {
          float curr_px = middle.at<float>(y, x);
          bool is_smaller = true;
          bool is_bigger = true;

          for (int i = y - maxsup_rad; i <= y + maxsup_rad; i++)
          {
            for (int j = x - maxsup_rad; j <= x + maxsup_rad; j++)
            {
              if (!((curr_px < middle.at<float>(i, j) || (y == i && x == j)) &&
                    (curr_px < down.at<float>(i, j)) &&
                    (curr_px < up.at<float>(i, j))))
              {
                is_smaller = false;
                break;
              }
            }
            if (!is_smaller)
            {
              break;
            }
          }
          for (int i = y - maxsup_rad; i <= y + maxsup_rad; i++)
          {
            for (int j = x - maxsup_rad; j <= x + maxsup_rad; j++)
            {
              if (!((curr_px > middle.at<float>(i, j) || (y == i && x == j)) &&
                    (curr_px > down.at<float>(i, j)) &&
                    (curr_px > up.at<float>(i, j))))
              {
                is_bigger = false;
                break;
              }
            }
            if (!is_bigger)
            {
              break;
            }
          }
          if (is_smaller || is_bigger)
          {
            if (refine_px)
            {
              KeyPoints kp_aux = interpExtremum(dog, o, s, y, x, NUM_SCALES - 1);
              kp_aux.resp = curr_px;

              int int_min = std::numeric_limits<int>::min();
              float float_min = std::numeric_limits<float>::min();

              if ((kp_aux.x == float_min && kp_aux.y == float_min) &&
                  (kp_aux.scale == int_min && kp_aux.octave == int_min))
              {
                kp.push_back({float(y), float(x), curr_px, o, s});
              }
              else
              {
                kp.push_back(kp_aux);
              }
            }
            else
            {
              kp.push_back({float(y), float(x), curr_px, o, s});
            }
          }
        }
      }
    }
  }
  std::cout << " ## SIFT > > > Total amount of Keypoints Founded: " << kp.size() << "." << std::endl;
}

/**
 * Applies threshold on minimum contrast value to keypoints to be accepted
 * @param dog DoG scale space pyramid
 * @param kp array where keypoints are stored
 * @param contrast_th threshold value to contrast threshold
**/
void contrastTh(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], std::vector<KeyPoints> &kp,
                float contrast_th)
{
  std::vector<KeyPoints> kp_aux;

  for (int i = 0; i < kp.size(); i++)
  {
    if (kp[i].resp >= contrast_th)
    {
      kp_aux.push_back(kp[i]);
    }
  }
  kp.clear();
  kp = kp_aux;
  std::cout << " ## SIFT > > > " << kp.size() << " Keypoints left..." << std::endl;
}

/**
 * Applies threshold on minimum edge value to KeyPoints to be accepted
 * @param dog DoG scale space pyramid
 * @param kp array where keypoints are stored
 * @param curv_th threshold value to curvature threshold
**/
void edgeTh(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], std::vector<KeyPoints> &kp,
            float curv_th)
{
  std::vector<KeyPoints> kp_aux;
  curv_th = (curv_th + 1) * (curv_th + 1) / curv_th;

  for (int i = 0; i < kp.size(); i++)
  {
    float dxx, dyy, dxy;
    float trH, detH, curv_ratio;

    cv::Mat D = dog[kp[i].octave][kp[i].scale];

    int y = kp[i].y;
    int x = kp[i].x;

    dxx = D.at<float>(y - 1, x) + D.at<float>(y + 1, x) - 2.0 * D.at<float>(y, x);
    dyy = D.at<float>(y, x - 1) + D.at<float>(y, x + 1) - 2.0 * D.at<float>(y, x);
    dxy = 0.25 * (D.at<float>(y - 1, x - 1) + D.at<float>(y + 1, x + 1) -
          D.at<float>(y + 1, x - 1) - D.at<float>(y - 1, x + 1));

    trH = dxx * dyy;
    detH = (dxx * dyy) - (dxy * dxy);

    curv_ratio = trH * trH / detH;

    if ( (detH > 0) && (curv_ratio < curv_th) )
    {
      kp_aux.push_back(kp[i]);
    }
  }
  kp.clear();
  kp = kp_aux;
}

void edgeThreshold(cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1], std::vector<KeyPoints> &kp,
            float curv_th)
{
  std::vector<KeyPoints> auxKp;
  cv::Mat Ix[NUM_OCTAVES][NUM_SCALES - 1], Iy[NUM_OCTAVES][NUM_SCALES - 1],
          Ix2[NUM_OCTAVES][NUM_SCALES - 1], Iy2[NUM_OCTAVES][NUM_SCALES - 1],
          Ixy[NUM_OCTAVES][NUM_SCALES - 1], response[NUM_OCTAVES][NUM_SCALES - 1];

  // INITIALIZING VARIABLES
  std::cout << " ## SIFT > > > Computing variables..." << std::endl;
  for(int i = 0; i < NUM_OCTAVES; i++)
  {
    for(int j = 0; j < NUM_SCALES - 1; j++)
    {
      cv::Sobel(dog[i][j], Ix[i][j], CV_32F, 1, 0, 7, 1, 0, cv::BORDER_DEFAULT);
      cv::Sobel(dog[i][j], Iy[i][j], CV_32F, 0, 1, 7, 1, 0, cv::BORDER_DEFAULT);
      
      Ix2[i][j] = Ix[i][j].mul(Ix[i][j]); // Ix^2
      Iy2[i][j] = Iy[i][j].mul(Iy[i][j]); // Iy^2
      Ixy[i][j] = Ix[i][j].mul(Iy[i][j]); // Ix * Iy

      response[i][j] = cv::Mat::zeros(cv::Size(dog[i][j].cols, dog[i][j].rows), CV_32F);
      for(int row = 0; row < dog[i][j].rows; row++)
      {
        for(int col = 0; col < dog[i][j].cols; col++)
        {
          float fx2 = Ix2[i][j].at<float>(row, col);
          float fy2 = Iy2[i][j].at<float>(row, col);
          float fxy = Ixy[i][j].at<float>(row, col);
          float det = (fx2 * fy2) - (fxy * fxy);
          float trace = (fx2 + fy2);
          response[i][j].at<float>(row, col) = det - curv_th * (trace*trace);
          //if(response.at<float>(row, col) < 0) response.at<float>(row, col) = 0;
          //printf("%f", response.at<float>(row, col));
        }
      }
      
      int count = 0;
      for(int row = 0; row < dog[i][j].rows; row++)
      {
        for(int col = 0; col < dog[i][j].cols; col++)
        {
          float val = response[i][j].at<float>(row, col);
          if(val < -1e4f)
          {
            response[i][j].at<float>(row, col) = 0.0f;
            count++;
          }
        }
      }

      std::cout << " ## SIFT > > > ########## dog[" << i << "][" << j << "], values to zero: " << count << ", total: " << dog[i][j].cols*dog[i][j].rows << std::endl;
    }
  }
  std::cout << " ## SIFT > > > Variables computed." << std::endl;
  
  std::cout << " ## SIFT > > > Thresholding Keypoints..." << std::endl;
  for(int i = 0; i < kp.size(); i++)
  {
    int oct = kp[i].octave, scl = kp[i].scale;
      int y = kp[i].y, x = kp[i].x;
      if(response[oct][scl].at<float>(y, x) != 0.0f)
      {
        auxKp.push_back( kp[i] );
      }
  }
  kp = auxKp;
  std::cout << " ## SIFT > > > Edge Threshold end." << std::endl;
}


/**
 * Initializes the scale scpace to calculate difference of gaussian (DoG) images.
 * It consists in a matrix of NUM_OCTAVES rows and NUM_SCALES columns
 * @param img original image which keypoints must be extracted
 * @param scales matrix containing pointers to each image of scale space
 * @param mgauss gaussian filter convolution window size
 * @param is_hdr flag to enable hdr functions
 * @param cv_size coefficient of variance mask size
**/
void dogInitScales( cv::Mat img, cv::Mat scales[NUM_OCTAVES][NUM_SCALES], bool isHDR,
                    int mgauss, int cv_size = CV_SIZE )
{
  cv::Mat img_aux;
  float k[] = {0.707107, 1.414214, 2.828428, 5.656856};

  mapPixelValues( img, img, MAPPING_INTERVAL_FLOAT_0_1 );
  cv::GaussianBlur(img, img, cv::Size(5, 5), 0, 0, cv::BORDER_DEFAULT);

  if ( isHDR )
  {
    cv::Mat img_cv, img_log;
    
    mapPixelValues( img, img, MAPPING_INTERVAL_FLOAT_0_1 );
    coefficienceOfVariationMask( img, img_cv );
    logTranformUchar( img_cv, img_log );
    img_log.convertTo( img_aux, CV_32FC1 );

    mapPixelValues( img, img );
  }
  else
  {
    mapPixelValues( img, img_aux );
  }

  for (int i = 0; i < NUM_OCTAVES; i++)
  {
    float ko = k[i];
    for (int j = 0; j < NUM_SCALES; j++)
    {
      cv::GaussianBlur(img_aux, scales[i][j], cv::Size(21, 21), ko, ko, cv::BORDER_DEFAULT);
      ko = ko * SQRT_2;
    }
    cv::resize(img_aux, img_aux, cv::Size(img_aux.cols/2, img_aux.rows/2));
  }
}

/**
 * Calculates the DoG images using the scale space
 * @param scales matrix containing pointers to each image of scale space
 * @param dog DoG scale space pyramid
**/
void dogCalc(cv::Mat scales[NUM_OCTAVES][NUM_SCALES],
             cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1])
{
  for (int o = 0; o < NUM_OCTAVES; o++)
  {
    for (int s = 0; s < NUM_SCALES - 1; s++)
    {
      dog[o][s] = cv::Mat::zeros(scales[o][s].size(), CV_32FC1);
      //cv::subtract(scales[o][s], scales[o][s + 1], dog[o][s]);

      for( int rows = 0; rows < scales[o][s].rows; rows++ )
      {
        for( int cols = 0; cols < scales[o][s].cols; cols++ )
        {
          float f = scales[o][s].at<float>(rows, cols) - scales[o][s+1].at<float>(rows, cols);
          dog[o][s].at<float>(rows, cols) = f;
        }
      }

      //std::string imgName = "img_dog_oct" +std::to_string(o) +"_scl" +std::to_string(s) +".png";
      //cv::imwrite("out/"+imgName, dog[o][s]);
    }
  }
}

/**
 * Coordenates the DoG detector execution with HDR functions
 * @param img grayscale image to extract feature points (keypoints)
 * @param kp array of keypoints detected
 * @param refine_px flag to enable subpixel interpolation
 * @param mgauss gaussian filter size
 * @param maxsup_size maximum number of checks when locating maxima/minima in DoG images
 * @param contrast_th contrast threshold value to threshold proccess
 * @param curv_th edge threshold value to threshold process
 * @param cv_size mask size to compute coefficient of variation
**/
void dogKp(cv::Mat img, std::vector<KeyPoints> &kp, bool isHDR, bool refine_px, 
           int mgauss, int maxsup_size, float contrast_th, float curv_th, int cv_size )
{
  cv::Mat scales[NUM_OCTAVES][NUM_SCALES];
  cv::Mat dog[NUM_OCTAVES][NUM_SCALES - 1];
  cv::Mat img_norm;

  mapPixelValues( img, img_norm );
  //img.copyTo( img_norm );

  std::cout << " ## SIFT > > Mounting Scale Space..." << std::endl;
  dogInitScales(img_norm, scales, isHDR, mgauss);
  
  std::cout << " ## SIFT > > Calculating Difference of Gaussians..." << std::endl;
  dogCalc(scales, dog);

  std::cout << " ## SIFT > > Computing Maxima Suppression and subpixel keypoint coordinates..." << std::endl;
  dogMaxSup(dog, kp, maxsup_size, curv_th, refine_px);
  //plotKeyPoints( img, kp, "out/img_kps_total.png", kp.size() );
  
  std::cout << " ## SIFT > > Contrast Threshold..." << std::endl;
  contrastTh(dog, kp, contrast_th);
  //plotKeyPoints( img, kp, "out/img_kps_contrast_th.png", kp.size() );
  
  std::cout << " ## SIFT > > Edge Threshold..." << std::endl;
  edgeTh(dog, kp, curv_th);
  //edgeThreshold(dog, kp, curv_th);
  
  //plotKeyPoints( img, kp, "out/img_kps_edge_th.png", kp.size() );
}
