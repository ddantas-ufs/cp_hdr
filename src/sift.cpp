#include "../include/descriptors/sift.h"

cv::Mat repeatLastRow( cv::Mat& mat )
{
  cv::Mat ret = cv::Mat( mat.rows, mat.cols, mat.type(), cv::Scalar(0) );
  int rows = mat.rows;
  int cols = mat.cols;

  for( int i=1; i<rows; i++ )
  {
    for( int j=0; j<cols; j++ )
    {
      ret.at<float>(i-1, j) = mat.at<float>(i, j);
    }
  }

  for( int j=0; j<cols; j++ )
  {
    ret.at<float>(rows-1, j) = mat.at<float>(rows-1, j);
  }

  return ret;
}

cv::Mat repeatFirstRow( cv::Mat& mat )
{
  cv::Mat ret = cv::Mat( mat.rows, mat.cols, mat.type(), cv::Scalar(0) );
  int rows = mat.rows;
  int cols = mat.cols;

  for( int j=0; j<cols; j++ )
  {
    ret.at<float>(0, j) = mat.at<float>(0, j);
  }

  for( int i=0; i<rows-1; i++ )
  {
    for( int j=0; j<cols; j++ )
    {
      ret.at<float>(i+1, j) = mat.at<float>(i, j);
    }
  }

  return ret;
}

cv::Mat repeatFirstColumn( cv::Mat& mat )
{
  cv::Mat ret = cv::Mat( mat.rows, mat.cols, mat.type(), cv::Scalar(0) );
  int rows = mat.rows;
  int cols = mat.cols;

  for( int i=0; i<rows; i++ )
  {
    ret.at<float>(i, 0) = mat.at<float>(i, 0);
  }

  for( int i=0; i<rows; i++ )
  {
    for( int j=0; j<cols-1; j++ )
    {
      ret.at<float>(i, j+1) = mat.at<float>(i, j);
    }
  }

  return ret;
}

cv::Mat repeatLastColumn( cv::Mat& mat )
{
  cv::Mat ret = cv::Mat( mat.rows, mat.cols, mat.type(), cv::Scalar(0) );
  int rows = mat.rows;
  int cols = mat.cols;

  for( int i=0; i<rows-1; i++ )
  {
    for( int j=1; j<cols; j++ )
    {
      ret.at<float>(i, j-1) = mat.at<float>(i, j);
    }
  }

  for( int i=0; i<rows; i++ )
  {
    ret.at<float>(i, cols-1) = mat.at<float>(i, cols-1);
  }

  return ret;
}

/**
 * Create a Gaussian Kernel based on sigma value
 * 
 * @param sigma
 * @return array with Gaussian distribution based on sigma return a 2-dimensional array
 *         with arbitrary sizes is not efficient and can leave us with memory problems. 
 *         Our workaround is return an 1-dimensional array instead.
 *         
 *         In this way. to access the item arr[i][j], we just : arr[i*size+j].
 *         Where size is the 'j' size. 
**/
void gaussianKernel( float sigma, cv::Mat& kernel )
{
  int size = (int) 2 * (std::ceil( 3 * sigma ) + 1);
  int half = size / 2;

  double r, s = 2.0 * sigma * sigma;
  double sum = 0.0; // to normalize

  kernel = cv::Mat( cv::Size(size, size), CV_32F, cv::Scalar(0.0) );

  for( int x = -half; x <= half; x++ )
  {
    for( int y = -half; y <= half; y++ )
    {
        r = std::sqrt(x * x + y * y);
        kernel.at<float>(x + half, y+half) = (std::exp(-(r * r) / s)) / (M_PI * s);
        sum += kernel.at<float>(x + half, y+half);
    }
}

  // performing normalization
  for (int i = 0; i < size; ++i)
  {
    for (int j = 0; j < size; ++j)
    {
      sum += kernel.at<float>(i, j);
    }
  }
}


/**
 * Verify if the coorduinates (x, y) outside from img limits
 * 
 * @param img image
 * @param x coordinate x (column, width)
 * @param y coordinate y (row, height)
 * 
 * @return true if is outside from img or false
**/
bool isOut(cv::Mat img, int x, int y)
{
  int rows = img.rows;
  int cols = img.cols;
  return ( x <= 0 or x >= cols-1 or y <= 0 or y >= rows-1 );
}

void cartToPolarGradient( int dx, int dy, double mt[2] )
{
  double mag, theta;

  mag = std::sqrt( (dx+dx)+(dy*dy) );
  theta = (std::atan2( dy, dx ) * 180 / M_PI) - 90;

  mt[0] = mag;
  mt[1] = theta;
}

void cartToPolarGradientMat( cv::Mat& dx, cv::Mat& dy, cv::Mat& m, cv::Mat& theta )
{
  for( int i=0; i<dx.rows; i++ )
  {
    for( int j=0; j<dx.cols; j++ )
    {
      double ret[2];
      cartToPolarGradient( dx.at<int>(i, j), dy.at<int>(i, j), ret );
      m.at<double>(i, j) = ret[0];
      theta.at<double>(i, j) = ret[1];
    }
  }
}

void getGradient( cv::Mat& img, int x, int y, double mt[2] )
{
  int dy, dx;
  double hip;

  dy = ( ( int ) img.at<float>( std::min( img.rows-1, y+1 ), x ) ) 
     - ( ( int ) img.at<float>( std::max( 0, y-1 ) ), x );
  dx = ( ( int ) img.at<float>( y, std::min( img.cols-1, x+1 ) ) ) 
     - ( ( int ) img.at<float>( y, std::max( 0, x-1 ) ) );

  cartToPolarGradient( dx, dy, mt );
}

int quantizeOrientation( double theta, int bins )
{
  float width = std::floor( 360 / bins );
  return ( int ) std::floor( std::floor( theta ) / width );
}

/**
 * Method that calculates the orientation of a given KeyPoint
 * 
 * @param img source image of keypoint being calculated
 * @param kp KeyPoint to be used to calculate orientation
 * 
 * @return the orientation of the keypoint
**/
std::vector<KeyPoints> calcOrientation( cv::Mat& img, KeyPoints kp )
{
  cv::Mat kernel;
  std::vector<KeyPoints> auxList;
  float sigma, maxIndex, maxValue, hist[DESC_HIST_BINS];//, *kernel;
  int radius, sizeKernel;

  sigma = DESC_GAUSS_SIGMA * kp.resp;
  radius = (int) 2 * (std::ceil( sigma ) + 1);
  
  gaussianKernel( sigma, kernel );

  for( int i = -radius; i < radius; i++ )
  {
    int y, binn;
    double mt[2], weight;

    y = kp.y + i;

    if( isOut(img, 1, y) )
      continue;

    for( int j = -radius; j < radius; j++ )
    {
      int x = kp.x + j;
      if( isOut(img, x, 1) )
        continue;

      // mt[0] = mag and mt[1] = theta
      getGradient( img, x, y, mt );
      
      weight = kernel.at<float>(i+radius, j+radius) * mt[0];
      binn = quantizeOrientation(mt[1], DESC_HIST_BINS) - 1;
      hist[binn] += weight;
    }
  }

  // Getting max value and respective index 
  maxValue = std::numeric_limits<float>::min();
  maxIndex = std::numeric_limits<float>::min();
  for( int i = 0; i < DESC_HIST_BINS; i++ )
  {
    if( hist[i] > maxValue )
    {
      maxValue = hist[i];
      maxIndex = i;
    }
  }
  
  kp.direction = maxIndex * 10;

  // If there's values above 85% of max value, return them.
  for( int i = 0; i < DESC_HIST_BINS; i++ )
  {
    if( i == maxIndex )
      continue;
    
    if( hist[i] > ( 0.85 * maxValue ) )
    {
      KeyPoints aux;
      aux.x = kp.x;
      aux.y = kp.y;
      aux.resp = kp.resp;
      aux.scale = kp.scale;
      aux.octave = kp.octave;
      aux.direction = i * 10;
      auxList.push_back( aux );
    }
  }

  return auxList;
}

/**
 * Method that add keypoints orientation to kp.
 * 
 * @param kp KeyPoints vector
 * @param img image being descripted
 * @param mGauss Gaussian blur window size
 * @param sigma  Gaussian blur sigma value
**/
void siftKPOrientation( std::vector<KeyPoints> kp, cv::Mat& img, int mGauss, float sigma )
{
  std::vector<KeyPoints> auxList, newKp;
  cv::Mat img2blur, img2gray;

  cv::GaussianBlur( img, img2blur, cv::Size(mGauss, mGauss), sigma, sigma, 
                   cv::BORDER_REPLICATE );
  cv::cvtColor( img, img2gray, CV_BGR2GRAY );

  for( int i = 0; i < kp.size(); i++ ) 
  {
    int px, py;

    KeyPoints key = kp.at(i);
    auxList = calcOrientation(img, key);

    // ROUNDING 30 * cos( radians( octave ) ) AND sin( radians( octave ) )
    px = (int) 0.5 + ( 30 * ( std::cos( key.direction * ( M_PI / 180) ) ) );
    py = (int) 0.5 + ( 30 * ( std::sin( key.direction * ( M_PI / 180) ) ) );

    cv::arrowedLine( img2gray, cv::Point( key.x, key.y ),
                     cv::Point( key.x + px, key.y + py ), 
                     cv::Scalar( 0, 0, 255 ), 1 );
    for( int j = 0; j < auxList.size(); j++ )
    {
      int ppx, ppy;
      KeyPoints point = auxList.at(j);

      ppx = (int) 0.5 + ( 30 * ( std::cos( point.direction * ( M_PI / 180) ) ) );
      ppy = (int) 0.5 + ( 30 * ( std::sin( point.direction * ( M_PI / 180) ) ) );
      cv::arrowedLine( img2gray, cv::Point( point.x, point.y ), 
                       cv::Point( point.x+px, point.y+py ), 
                       cv::Scalar( 0, 0, 255 ), 1 );
    }
    newKp.insert( newKp.end(), auxList.begin(), auxList.end() );
  }
  
  kp.insert( kp.end(), newKp.begin(), newKp.end() ); 
}

void getPatchGrads( cv::Mat& subImage, cv::Mat& retX, cv::Mat& retY )
{
  cv::Mat r1 = repeatLastRow( subImage );
  cv::Mat r2 = repeatFirstRow( subImage );

  cv::subtract( r1, r2, retX );

  r1 = repeatLastColumn( subImage );
  r2 = repeatFirstColumn( subImage );
  
  cv::subtract( r1, r2, retY );
}

void siftExecuteDescription( std::vector<KeyPoints> kpList, cv::Mat& img, int bins,
                             float rad )
{
  std::vector<std::vector<std::vector<float>>> hist;
  int binWidth, windowSize, radius;
  float sigma, radVal;
  cv::Mat img2gray;

  cv::cvtColor( img, img2gray, CV_BGR2GRAY );

  windowSize = 16;
  sigma = windowSize/6;
  radius = std::floor(windowSize/2);
  binWidth = std::floor(360/bins);
  radVal = 180.0 / M_PI;

  // Creating float[4][4][bins] zeroed vector
  for( int i=0; i<4; i++ )
  {
    std::vector<std::vector<float>> histVec;
    for(int j=0; j<4; j++)
    {
      std::vector<float> histBin;
      for(int k=0; k<bins; k++)
      {
        histBin.push_back(0.0);
      }
      histVec.push_back(histBin);
    }
    hist.push_back(histVec);
  }

  for( int index=0; index < kpList.size(); index++ )
  {
    cv::Mat patch, dx, dy, mag, the, kernel, featVec;
    KeyPoints kp = kpList.at(index);
    float d, theta;// *auxKernel;
    int i, x, y, s, t, l, b, r, xIni, xFim, yIni, yFim, subW;

    i = 0;
    x = kp.x;
    y = kp.y;
    s = kp.scale;
    d = kp.direction;
    
    theta = M_PI * 180.0;
    gaussianKernel( sigma, kernel );

    t = std::max( 0, y-( (int) windowSize/2) );
    l = std::max( 0, x-( (int) windowSize/2) );

    b = std::max( img.rows, y-( (int) (windowSize/2)+1) );
    r = std::max( img.cols, x-( (int) (windowSize/2)+1) );

    patch = img(cv::Range(t, b), cv::Range(l, r));

    getPatchGrads( patch, dx, dy );

    if( dx.rows < windowSize+1 )
    {
      if( t == 0 )
      {
        xIni = kernel.rows-dx.rows;
        xFim = kernel.rows;
      } else
      {
        xIni = 0;
        xFim = dx.rows;
      }
    }
    if( dx.cols < windowSize+1 )
    {
      if( l == 0 )
      {
        yIni = kernel.cols-dx.cols;
        yFim = kernel.cols;
      } else
      {
        yIni = 0;
        yFim = dx.cols;
      }
    }

    kernel = kernel( cv::Range(xIni, xFim), cv::Range(yIni, yFim) );
    
    if( dy.rows < windowSize+1 )
    {
      if( t == 0 )
      {
        xIni = kernel.rows-dy.rows;
        xFim = kernel.rows;
      } else
      {
        xIni = 0;
        xFim = dy.rows;
      }
    }
    if( dy.cols < windowSize+1 )
    {
      if( l == 0 )
      {
        yIni = kernel.cols-dy.cols;
        yFim = kernel.cols;
      } else
      {
        yIni = 0;
        yFim = dy.cols;
      }
    }

    kernel = kernel( cv::Range(xIni, xFim), cv::Range(yIni, yFim) );

    mag = cv::Mat( cv::Size(dx.cols, dx.cols), CV_64F, cv::Scalar(0.0) ); // magnitude
    the = cv::Mat( cv::Size(dx.cols, dx.cols), CV_64F, cv::Scalar(0.0) ); // theta
    
    cartToPolarGradientMat( dx, dy, mag, the );

    if( ( sizeof(kernel) / sizeof(float) ) == 17 )
    {
      dx = dx.dot( kernel );
      dy = dy.dot( kernel );
    }

    subW = (int) windowSize / 4;
    featVec = cv::Mat( cv::Size(1, bins*windowSize), CV_32F, cv::Scalar(0.0) );

    for(int i=0; i<subW; i++)
    {
      for( int j=0; j<subW; j++ )
      {
        t = i*subW;
        l = j*subW;
        b = std::min( img.rows, (i+1) * subW );
        b = std::min( img.cols, (j+1) * subW );

        hist = getHistogramForSubregion();
      }
    }
  }

}

/**
 * SIFT MAIN METHOD
 * 
 * @param kp KeyPoints detected
 * @param name string with image's name
**/
int siftDescriptor( std::vector<KeyPoints> kp, cv::Mat img, cv::Mat imgGray, int mGauss,
                    float sigma )
{
  siftKPOrientation( kp, img, mGauss, sigma );
  siftExecuteDescription( kp, img, DESC_BINS, DESC_RADIUS );
  return 0;
}
