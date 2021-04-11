#include "../include/descriptors/sift.h"

/**
 * Return a flattened array of a cv::Mat object
 * 
 * @param mat cv::Mat matrix to be flatenned
 * @return the flatenned Matrix in a vector object
**/
void returnRavel( cv::Mat& mat, cv::Mat& flat )
{
  for (int i = 0; i < mat.rows; i++)
  {
    for( int j = 0; j < mat.cols; j++)
    {
      double item = mat.at<double>(i, j);
      flat.at<float>(i*mat.cols+j, 0) = (float) item;
    }
  }
  //flat = flat * 255; // (putting in 0-255 range)
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
bool isOut(cv::Mat &img, int x, int y)
{
  int rows = img.rows;
  int cols = img.cols;
  return ( x <= 0 || x >= cols-1 || y <= 0 || y >= rows-1 );
}

void repeatLastRow( cv::Mat &mat, cv::Mat &ret )
{
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
    ret.at<float>(rows-1, j) = mat.at<float>(rows-1, j);
}

void repeatFirstRow( cv::Mat &mat, cv::Mat &ret )
{
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
}

void repeatLastColumn( cv::Mat &mat, cv::Mat &ret )
{
  int rows = mat.rows;
  int cols = mat.cols;

  for( int i=0; i<rows; i++ )
  {
    for( int j=1; j<cols; j++ )
    {
      ret.at<float>(i, j-1) = mat.at<float>(i, j);
    }
  }

  for( int i=0; i<rows; i++ )
    ret.at<float>(i, cols-1) = mat.at<float>(i, cols-1);
}

void repeatFirstColumn( cv::Mat &mat, cv::Mat &ret )
{
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
}

/*
void repeatLastRow(cv::Mat mat, cv::Mat &ret)
{
  mat.row(mat.rows - 1).copyTo(ret.row(ret.rows - 1));
  mat.rowRange(1, mat.rows).copyTo(ret.rowRange(0, ret.rows - 1));
}

void repeatFirstRow(cv::Mat mat, cv::Mat &ret)
{
  mat.row(0).copyTo(ret.row(0));
  mat.rowRange(0, mat.rows - 1).copyTo(ret.rowRange(1, ret.rows));
}

void repeatLastColumn(cv::Mat mat, cv::Mat &ret)
{
  mat.col(mat.cols - 1).copyTo(ret.col(ret.cols - 1));
  mat.colRange(1, mat.cols).copyTo(ret.colRange(0, ret.cols - 1));
}

void repeatFirstColumn(cv::Mat mat, cv::Mat &ret)
{
  mat.col(0).copyTo(ret.col(0));
  mat.colRange(0, mat.cols - 1).copyTo(ret.colRange(1, ret.cols));
}
*/
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
  int size = (int) (2 * std::ceil( 3 * sigma )) + 1;
  int half = (int) size / 2;

  
  double r, s = 2.0 * sigma * sigma;
  double sum = 0.0; // to normalize

  kernel = cv::Mat( cv::Size(size, size), CV_32FC1, cv::Scalar(0.0) );

  for( int x = -half; x <= half; x++ )
  {
    for( int y = -half; y <= half; y++ )
    {
        r = std::sqrt( (x * x) + (y * y) );
        kernel.at<float>(x + half, y+half) = (float) (std::exp(-(r * r) / s)) / (M_PI * s);
        sum += (double) kernel.at<float>(x + half, y+half);
    }
  }

  // performing normalization
  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j)
      kernel.at<float>(i, j) = kernel.at<float>(i, j) / sum;
}

void gaussianKernel( int size, cv::Mat& kernel )
{
  int half = (int) size / 2;

  double sigma = 1.0;  
  double r, s = 2.0 * sigma * sigma;
  double sum = 0.0; // to normalize

  kernel = cv::Mat( cv::Size(size, size), CV_32FC1, cv::Scalar(0.0) );

  for( int x = -half; x <= half; x++ )
  {
    for( int y = -half; y <= half; y++ )
    {
        r = std::sqrt( (x * x) + (y * y) );
        kernel.at<float>(x + half, y+half) = (float) (std::exp(-(r * r) / s)) / (M_PI * s);
        sum += (double) kernel.at<float>(x + half, y+half);
    }
  }

  // performing normalization
  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j)
      kernel.at<float>(i, j) = kernel.at<float>(i, j) / sum;
}

/**
 * Cartesian to polar coordinate
 * 
 * @param dx x coordinate
 * @param dy y coordinate
 * @param dm return array with magnitude and theta
**/
void cartToPolarGradient( int dx, int dy, double mt[2] )
{
  mt[0] = std::sqrt( (dx*dx)+(dy*dy) );
  mt[1] = ( (std::atan2(dy, dx) +M_PI) * (180/M_PI) ) - 90;
}

/**
 * Cartesian to polar angle
 * 
 * @param dx x coordinate
 * @param dy y coordinate
 * @param m return array with magnitudes
 * @param theta return array with thetas
**/
void cartToPolarGradientMat( cv::Mat& dx, cv::Mat& dy, cv::Mat& m, cv::Mat& theta )
{
  std::cout << "---> cartToPolarGradientMat" << std::endl;
  for( int i=0; i<dx.rows; i++ )
  {
    for( int j=0; j<dx.cols; j++ )
    {
      double ret[2] = {0,0};
      int x = (int) dx.at<double>(i, j);
      int y = (int) dy.at<double>(i, j);

      cartToPolarGradient( x, y, ret );
      
      m.at<double>(i, j) = ret[0];
      theta.at<double>(i, j) = ret[1];
    }
  }
}

void getGradient( cv::Mat& img, int x, int y, double mt[2] )
{
  int dy, dx;

  dy = ( ( int ) img.at<float>( std::min( img.rows-1, y+1 ), x ) ) 
     - ( ( int ) img.at<float>( std::max( 0, y-1 ) ), x );
  dx = ( ( int ) img.at<float>( y, std::min( img.cols-1, x+1 ) ) ) 
     - ( ( int ) img.at<float>( y, std::max( 0, x-1 ) ) );

  cartToPolarGradient( dx, dy, mt );
}

int quantizeOrientation( double theta, int bins )
{
  float width = std::floor( 360 / bins );
  return ( int ) std::floor( theta ) / width;
}

/**
 * Method that calculates the orientation of a given KeyPoint
 * 
 * @param img source image of keypoint being calculated
 * @param kp KeyPoint to be used to calculate orientation
 * 
 * @return the orientation of the keypoint
**/
void calcOrientation( cv::Mat& img, KeyPoints kp, std::vector<KeyPoints> auxList )
{
  cv::Mat kernel, hist;
  float sigma, maxIndex, maxValue;
  int radius, sizeKernel;

  auxList.clear();

  hist = cv::Mat::zeros( cv::Size( SIFT_DESC_BINS_PER_SW, 1 ), CV_32FC1 );

  sigma = SIFT_DESC_ORIENT_SIGMA * kp.scale;
  radius = (int) (2 * std::ceil( sigma )) + 1;

  gaussianKernel( sigma, kernel );

  for( int i = -radius; i < radius+1; i++ )
  {
    int y, binn;
    double mt[2], weight;

    y = kp.y + i;

    if( !isOut(img, 1, y) )
    {
      for( int j = -radius; j < radius+1; j++ )
      {
        int x = kp.x + j;
        if( !isOut(img, x, 1) )
        {
          getGradient( img, x, y, mt );
          
          weight = kernel.at<float>(i+radius, j+radius) * mt[0];
          binn = quantizeOrientation(mt[1], SIFT_DESC_BINS_PER_SW) - 1;

          //std::cout << "---> x=" << x << ", y=" << y << std::endl;
          //std::cout << "---> mag=" << mt[0] << ", theta=" << mt[1] <<std::endl;
          //std::cout << "---> binn=" << binn << ", weight=" << weight << std::endl;
          //std::cout << "---> hist[binn]=" << hist.at<float>(binn, 0) << std::endl;
          //std::cout << "---> hist[binn]=" << hist[binn] << std::endl;

          if( !std::isnan(weight) )
            hist.at<float>(binn, 0) = hist.at<float>(binn, 0) + weight;

          //hist[binn] = hist[binn] + weight;
          //for(int z = 0; z<DESC_HIST_BINS; z++ ) std::cout << hist.at<float>(z, 0) << ", ";
          //for(int z = 0; z<DESC_HIST_BINS; z++ ) std::cout << hist[z] << " ";;
          //std::cout << std::endl;
        }
      }
    }
  }
  
  maxIndex = std::numeric_limits<float>::min();
  maxValue = std::numeric_limits<float>::min();
  for( int i = 0; i < SIFT_DESC_ORIENT_SIGMA; i++ )
  {
    if( hist.at<float>(i, 0) > maxValue )
    //if( hist[i] > maxValue )
    {
      maxValue = hist.at<float>(i, 0);
      //maxValue = hist[i];
      maxIndex = i;
    }
  }
  
  kp.direction = maxIndex * 10;
  std::cout << "kpIndex = " << maxIndex << std::endl;
  std::cout << "kpValue = " << maxValue << std::endl;
  std::cout << "keypoint:" << std::endl;
  std::cout << keypointToString(kp) << std::endl;

  // If there's values above 85% of max value, return them.
  for( int i = 0; i < SIFT_DESC_ORIENT_SIGMA; i++ )
  {
    if( i != maxIndex )
    {
      if( hist.at<double>(i, 0) > ( 0.85 * maxValue ) )
      //if( hist[i] > ( 0.85 * maxValue ) )
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
      //else 
      //{
      //  std::cout << "i: " << i << " so kp: " << std::endl;
      //}
    }
  }
  std::cout << "Returned auxList size:" << auxList.size() << std::endl;
}

/**
 * Method that add keypoints orientation to kp.
 * 
 * @param kp KeyPoints vector
 * @param img image being descripted
 * @param mGauss Gaussian blur window size
 * @param sigma  Gaussian blur sigma value
**/
void siftKPOrientation( std::vector<KeyPoints> kp, cv::Mat& img, int mGauss,
                        float sigma )
{
  std::vector<KeyPoints> auxList, newKp;
  //cv::Mat imgCopy;
  //img.copyTo( imgCopy );

  cv::GaussianBlur( img, img, cv::Size(mGauss, mGauss), sigma, sigma, 
                   cv::BORDER_REPLICATE );

  for( int i = 0; i < kp.size(); i++ ) 
  {
    int px, py;

    KeyPoints key = kp[i];
    calcOrientation(img, key, auxList);
    
    //  30 * cos( radians( degrees ) ) AND sin( radians( degrees ) )
    px = (int) ( 30 * ( std::cos( key.direction * ( M_PI / 180) ) ) );
    py = (int) ( 30 * ( std::sin( key.direction * ( M_PI / 180) ) ) );

    //cv::arrowedLine( imgCopy, cv::Point( key.x, key.y ),
    //                 cv::Point( key.x + px, key.y + py ),
    //                 cv::Scalar( 0, 0, 255 ), 1 );

    for( int j = 0; j < auxList.size(); j++ )
    {
      KeyPoints point = auxList[j];

      px = (int) ( 30 * ( std::cos( point.direction * ( M_PI / 180) ) ) );
      py = (int) ( 30 * ( std::sin( point.direction * ( M_PI / 180) ) ) );
      //cv::arrowedLine( imgCopy, cv::Point( point.x, point.y ), 
      //                 cv::Point( point.x+px, point.y+py ), 
      //                 cv::Scalar( 0, 0, 255 ), 1 );
    }
    // APPEND newKp
    for( int k=0; k<auxList.size(); k++ )
    {
      newKp.push_back( auxList[k] );
    }
  }
  //std::cout << " append kp " << std::endl;
  for( int k=0; k<newKp.size(); k++ )
  {
    kp.push_back( newKp[k] );
  }
  
  std::cout << " fim siftKPOrientation " << std::endl;
}

void getPatchGrads( cv::Mat& subImage, cv::Mat& retX, cv::Mat& retY )
{
  std::cout << "getPatches subImage size: " << subImage.size() << std::endl;
  cv::Mat r1 = cv::Mat( subImage.size(), subImage.type() );
  cv::Mat r2 = cv::Mat( subImage.size(), subImage.type() );

  repeatLastRow( subImage, r1 );
  repeatFirstRow( subImage, r2 );

  cv::subtract( r1, r2, retX );

  repeatLastColumn( subImage, r1 );
  repeatFirstColumn( subImage, r2 );
  
  cv::subtract( r1, r2, retY );
}

/**
 * convert an 1d array coordinate into a 2d array coordinates.
 * 
 * @param index the index of 1d array coordinated
 * @param rows the amount of rows in the 2d array
 * @param cols the amount of columns in the 2d array
 * @param ret array containing the index coordinate mapped into 2d array coordinates
 *            where ret[0]=rows and ret[1]=cols
 * 
 * Similar to numpy.unravel_index() function
 * https://numpy.org/doc/stable/reference/generated/numpy.unravel_index.html
 * 
**/
void unravelIndex( int index, int rows, int cols, int ret[2] )
{
  for( int i=0; i<rows; i++ )
    for( int j=0; j<cols; j++ )
      if( i+j == index )
      {
        ret[0] = i;
        ret[1] = j;
        break;
      }
}

/**
 * Return the histogram of a given subregion
 * 
 * @param mag the subregion's magnitudes
 * @param theta subregion's angles
 * @param numBin amount of values to be returned
 * @param refAngle angle of reference to calculate the histogram
 * @param binWidth width of the bin
 * @param subW width of the subregion
 * @param hist Matrix where the histogram is being returned
**/
void getHistogramForSubregion( cv::Mat& mag, cv::Mat& theta, int numBin, int refAngle,
                               int binWidth, int subW, cv::Mat& hist )
{
  double minimum = 0.000001;
  float center = (subW/2) - 0.5;
  hist = cv::Mat::zeros( cv::Size(numBin, 1), CV_32FC1 );

  cv::Mat arrMag = cv::Mat::zeros( cv::Size(mag.rows*mag.cols, 1), CV_32FC1 );
  cv::Mat arrThe = cv::Mat::zeros( cv::Size(theta.rows*theta.cols, 1), CV_32FC1 );
  std::cout << "-> arrMag size: " << mag.size() << std::endl;
  std::cout << "-> mag cols: " << mag.cols << "mag rows: " << mag.rows << std::endl;

  //std::cout << "-> mag size: " << mag.size() << std::endl;
  returnRavel( mag, arrMag );
  //std::cout << "-> arrMag: " << arrMag << std::endl;
  
  //std::cout << "-> theta size: " << theta.size() << std::endl;
  returnRavel( theta, arrThe );
  //std::cout << "-> arrTheta: " << arrThe << std::endl;

  for( int i=0; i<arrMag.cols; i++ )
  {
    double mg = arrMag.at<float>(i, 0);
    int angle = (int) (arrThe.at<float>(i, 0)-refAngle) % 360;
    int b = quantizeOrientation(angle, numBin);
    double vote = mg;

    // b*binWidth is the start angle of the histogram bin
    // b*binWidth+binWidth/2 is the center of the histogram bin
    // angle -[...] is the distance from the angle to the center of the bin 
    double histInterpWeight = 1-std::abs(angle-((b*binWidth)+(binWidth/2)))/(binWidth/2);
    vote = vote * std::max( histInterpWeight, minimum );

    int idx[2];
    double xInterpWeight, yInterpWeight;
    unravelIndex( i, subW, subW, idx );

    xInterpWeight = std::max( (double) 1-(std::abs(idx[0]-center)/center), minimum );
    yInterpWeight = std::max( (double) 1-(std::abs(idx[1]-center)/center), minimum );

    vote = vote * (xInterpWeight * yInterpWeight);
    hist.at<float>(b, 0) += (float) vote;
    //std::cout << "--> vote: " << vote << std::endl;
  }
  std::cout << "--> histSubregion: ";
  for(int k = 0; k<numBin; k++) std::cout << hist.at<float>(k, 0) << ", ";
  std::cout << std::endl;
}

void siftExecuteDescription( std::vector<KeyPoints> &kpList, cv::Mat &img )
{
  cv::Mat siftWindow, kernel;

  // Calculates a 17x17 Gaussian kernel
  gaussianKernel( SIFT_DESC_WINDOW+1, kernel );

  for( int i = 0; i < kpList.size(); i++ )
  {
    cv::Mat mag, the, dx, dy;
    int swHalf = (int) SIFT_DESC_WINDOW / 2;

    // Generating 17x17 window (16x16 window) + keypoint's row and column
    siftWindow = cv::Mat::zeros( SIFT_DESC_WINDOW+1, SIFT_DESC_WINDOW+1, CV_32FC1 );
    int swRows = 0;
    for( int rows = kpList[i].y-swHalf; rows < kpList[i].y+swHalf; rows++ )
    {
      int swCols = 0;
      for( int cols = kpList[i].x-swHalf; cols < kpList[i].x+swHalf; cols++ )
      {
        int x = cols, y = rows;

        // Extrapolating image borders        
        if( x > img.cols ) x = img.cols-(x - img.cols)-1;
        if( y > img.rows ) y = img.rows-(y - img.rows)-1;
        if( x < 0 ) x = std::abs( x );
        if( y < 0 ) y = std::abs( y );

        //std::cout << "----> X: " << x << ", Y: " << y << std::endl;
        //std::cout << "----> swRows: " << swRows << ", swCols: " << swCols << std::endl;
        siftWindow.at<float>(swRows, swCols) = img.at<float>(y, x);
        swCols = swCols + 1;
      }
      swRows = swRows + 1;
    }

    // Calculating magnitude and angle of pixels
    std::cout << "getPatchGrads" << std::endl;
    getPatchGrads( siftWindow, dx, dy );
    std::cout << "getPatchGrads ok" << std::endl;

    mag = cv::Mat::zeros( cv::Size(SIFT_DESC_WINDOW+1, SIFT_DESC_WINDOW+1), CV_64FC1 ); // magnitude
    the = cv::Mat::zeros( cv::Size(SIFT_DESC_WINDOW+1, SIFT_DESC_WINDOW+1), CV_64FC1 ); // theta
    
    std::cout << "cartToPolarGradientMat" << std::endl;
    cartToPolarGradientMat( dx, dy, mag, the );

    // pixels that are closer to keypoint should have stronger values
    siftWindow = siftWindow * kernel;
    
    // dividir janela em 16 subjanelas 4x4.
    for( int rows = 0; rows < SIFT_DESC_SW_QTD; rows++ )
    {
      for( int cols = 0; cols < SIFT_DESC_SW_QTD; cols++ )
      {
        cv::Mat hist, subMag, subThe;
        // LEMBRAR DE AJUSTAR PARA NAO PEGAR LINHA/COLUNA DO KP
        subMag = mag( cv::Range(rows, rows+SIFT_DESC_SW_SIZE), 
                      cv::Range(cols, cols+SIFT_DESC_SW_SIZE) );
        subThe = the( cv::Range(rows, rows+SIFT_DESC_SW_SIZE), 
                      cv::Range(cols, cols+SIFT_DESC_SW_SIZE) );
        
        // calculate 8-bin histogram for subwindow
        getHistogramForSubregion( subMag, subThe, SIFT_DESC_BINS_PER_SW, kpList[i].scale,
                                  360/SIFT_DESC_BINS_PER_SW, SIFT_DESC_SW_QTD, hist );
        
        // adding each bin value to descriptor
        for(int idx = 0; idx<SIFT_DESC_BINS_PER_SW; idx++)
        {
          float hst = hist.at<float>(idx, 0);
          kpList[i].descriptor.push_back( hst );
        }
      }
    }
    std::cout << std::endl << "descritor calculado " << i << std::endl;
    for( int k=0; k<kpList[i].descriptor.size(); k++ ) std::cout << kpList[i].descriptor[k] << ", ";
    std::cout << std::endl;

    // para cada janela, calcular histograma de 8 amostras

    // adicionar essas 8 amostras como valores no descritor do keypoint

  }
  //dx.release();
  //dy.release();
  //siftWindow.release();
  //kernel.release();
  //mag.release();
  //the.release();
  //subMag.release();
  //subThe.release();
}

/**
 * Calculates de description of the Keypoints
 * 
 * @param kpList list of Keypoints to calculate the description
 * @param img image from wich the keypoints description are being calculated
 * @param bins amount of bins
 * @param rad radius used to calculate the bubble around the kpList KeyPoints
 * @param ret vector where the calculated descriptor will be returned
**/
void siftExecuteDescription( std::vector<KeyPoints> kpList, cv::Mat& img, int bins,
                             float rad )
{
  int binWidth, windowSize, radius;//, x, y, s, t, l, b, r, subW;
  //cv::Mat patch, dx, dy, mag, the, kernel, featVec;
  //cv::Mat featVec;
  float sigma;//, d, theta;
  //KeyPoints kp;

  windowSize = 16; // receber como parametro (imp, raio)
  sigma = windowSize/6;
  radius = (int) windowSize/2;
  binWidth = (int) 360/bins;

  std::cout << "for kpList.size()" << std::endl;
  for( int index=0; index < kpList.size(); index++ )
  {
    cv::Mat patch, dx, dy, mag, the;//, kernel;//, featVec;
    float d, theta;
    int x, y, s, t, l, b, r, subW;

    KeyPoints kp = kpList.at(index);
    kp.descriptor.resize(bins*windowSize);// = cv::Mat::zeros( cv::Size(, 1), CV_32FC1 );
    
    x = kp.x;
    y = kp.y;
    s = kp.scale;
    d = kp.direction;
    
    std::cout << "gaussianKernel" << std::endl;
    theta = M_PI * 180.0;
    //gaussianKernel( sigma, kernel );

    t = std::max( 0, y-( (int) std::floor(windowSize/2) ) );
    l = std::max( 0, x-( (int) std::floor(windowSize/2) ) );

    b = std::min( img.rows, y+((int) std::floor(windowSize/2))+1 );
    r = std::min( img.cols, x+((int) std::floor(windowSize/2))+1 );

    std::cout << "t: " << t << " l: " << l << std::endl;
    std::cout << "b: " << b << " r: " << r << std::endl;

    patch = img(cv::Range(t, b), cv::Range(l, r));
    //patch.convertTo(patch, CV_32FC1);
    //std::cout << "patch: " << patch << std::endl;
    std::cout << "patch size: " << patch.size() << std::endl;

    std::cout << "getPatchGrads" << std::endl;
    getPatchGrads( patch, dx, dy );
    std::cout << "getPatchGrads ok" << std::endl;

    //std::cout << "Kernel size: " << kernel.size() << std::endl;
    std::cout << "dx size: " << dx.size() << std::endl;
    std::cout << "dy size: " << dy.size() << std::endl;
    /*
    if( dx.rows < windowSize+1 )
    {
      if( t == 0 ) kernel = kernel( cv::Range(kernel.rows-dx.rows, kernel.rows), cv::Range(0, kernel.cols) );
      else kernel = kernel( cv::Range(0, dx.rows), cv::Range(0, kernel.cols) );
    }
    if( dx.cols < windowSize+1 )
    {
      if( l == 0 ) kernel = kernel( cv::Range(kernel.cols-dx.cols, kernel.rows), cv::Range(0, kernel.cols) );
      else kernel = kernel( cv::Range(0, dx.cols), cv::Range(0, kernel.cols) );
    }

    std::cout << "subspace kernel 1" << std::endl;
    std::cout << "kernel.size(): " << kernel.size() << std::endl;
    */
    /*// NÃO PRECISA CHECAR PRA dy, POIS dx e dy TEM O MESMO TAMANHO
    if( dy.rows < windowSize+1 )
    {
      if( t == 0 ) kernel = kernel( cv::Range(kernel.rows-dy.rows, kernel.rows), cv::Range(0, kernel.cols) );
      else kernel = kernel( cv::Range(0, dy.rows), cv::Range(0, kernel.cols) );
    }
    if( dy.cols < windowSize+1 )
    {
      if( l == 0 ) kernel = kernel( cv::Range(kernel.cols-dy.cols, kernel.rows), cv::Range(0, kernel.cols) );
      else kernel = kernel( cv::Range(0, dy.cols), cv::Range(0, kernel.cols) );
    }

    std::cout << "subspace kernel 2" << std::endl;
    //std::cout << "kernel: " << kernel << std::endl;
    std::cout << "kernel.size(): " << kernel.size() << std::endl;
    */

    mag = cv::Mat::zeros( cv::Size(dx.cols, dx.cols), CV_64FC1 ); // magnitude
    the = cv::Mat::zeros( cv::Size(dx.cols, dx.cols), CV_64FC1 ); // theta
    
    std::cout << "cartToPolarGradientMat" << std::endl;
    cartToPolarGradientMat( dx, dy, mag, the );

    //std::cout << "-> dx1: " << dx << std::endl;
    //std::cout << "-> dy1: " << dy << std::endl;

    //if( /*( sizeof(kernel) / sizeof(float) )*/ kernel.cols == 17 )
    //{
    //  dx = dx.dot( kernel );
    //  dy = dy.dot( kernel );
    //}

    //std::cout << "-> dx2: " << dx << std::endl;
    //std::cout << "-> dy2: " << dy << std::endl;
    //std::cout << "-> mag: " << mag << std::endl;
    //std::cout << "-> the: " << the << std::endl;

    std::cout << "create featVec" << std::endl;
    subW = std::floor(windowSize/4);
    //featVec = cv::Mat::zeros( cv::Size(bins*windowSize, 1), CV_32FC1 );

    for(int i=0; i<subW; i++)
    {
      for( int j=0; j<subW; j++ )
      {
        t = i*subW;
        l = j*subW;
        b = std::min( img.rows, (i+1)*subW );
        r = std::min( img.cols, (j+1)*subW );

        std::cout << " t: " << t << " b: "  << b << std::endl;
        std::cout << " l: " << l << " r: "  << r << std::endl;

        cv::Mat subMag = mag( cv::Range(t, b), cv::Range(l, r) );
        cv::Mat subThe = the( cv::Range(t, b), cv::Range(l, r) );

        //std::cout << "-> subMag: " << std::endl << mag << std::endl;
        //std::cout << "-> subThe: "<< std::endl << the << std::endl;

        std::cout << "getHistogramForSubregion" << std::endl;
        cv::Mat hist;
        getHistogramForSubregion( subMag, subThe, bins, s, binWidth, subW, hist );

        int indexIni = (i*subW*bins) + (j*bins);
        int indexEnd = (i*subW*bins) + ((j+1)*bins);
        
        std::cout << "populate featVec:" << std::endl;
        std::cout << "---> ini: " << indexIni << " fim: " << indexEnd << std::endl;
        for(int idx = indexIni; idx<indexEnd; idx++)
        {
          float hst = hist.at<float>(idx-indexIni, 0);
          //featVec.at<float>(idx, 0) = hst;
          kp.descriptor[idx] = hst;
          std::cout << hst << ", ";
        }
        std::cout << std::endl << "release subMag e subThe" << std::endl;
      }
        //std::cout << std::endl;
    }

    std::cout << "normalizando featVec" << std::endl;
    float norm = cv::norm( kp.descriptor, cv::NORM_L2, cv::noArray() );
    norm = cv::max( norm, (float)0.001 );
    for(int k=0; k<kp.descriptor.size(); k++)
    {
      kp.descriptor[k] /= norm;
      kp.descriptor[k] *= 255;
    }
    
    /*
    for(int idx=0; idx<featVec.cols; idx++)
    {
      float norm = cv::norm( featVec, cv::NORM_L2, cv::noArray() );
      //std::cout << "## norm " << norm << std::endl;
      featVec.at<float>(idx, 0) /= std::max((float)0.001, norm);
    }
    */
    //for(int idx=0; idx<featVec.cols; idx++)
    //  featVec.at<float>(idx, 0) *= 255;
    
    //std::cout << "push_back no vector de retorno, index = " << index << std::endl;
    //descriptorList.push_back(featVec.clone());

    //std::cout << "featVec size: " << featVec.cols << std::endl;
    //for(int k=0; k<featVec.cols; k++) std::cout << featVec.at<float>(k, 0) << ", ";
    //std::cout << std::endl;

    std::cout << "descriptor vec: " << kp.descriptor.size() << std::endl;
    for(int k=0; k<kp.descriptor.size(); k++) std::cout << kp.descriptor[k] << ", ";
    std::cout << std::endl;

    // copiando informacoes
    //descriptorList[index] = cv::Mat(featVec.size(), featVec.type());
    //featVec.copyTo( descriptorList[index] );
  }
  // std::cout << "saindo siftExecuteDescription. Size ret: " << descriptorList.size() << std::endl;  
}

/**
 * SIFT MAIN METHOD
 * 
 * @param kp KeyPoints detected
 * @param name string with image's name
**/
void siftDescriptor( std::vector<KeyPoints> &kp, cv::Mat& img_in, cv::Mat& img_gray,
                     int mGauss, float sigma )
{
  cv::Mat img_norm;
  if (img_in.depth() == 0)
  {
    img_in.convertTo(img_norm, CV_32FC1);
    img_norm = img_norm / 255.0;
  }
  else
  {
    img_in = img_norm / 256.0;
  }

  //removing blur applied in siftKPOrientation
  cv::cvtColor( img_norm, img_gray, CV_BGR2GRAY ); 

  std::cout << "Calculando orientações" << std::endl;
  siftKPOrientation( kp, img_gray, mGauss, sigma );
  
  std::cout << "KeyPoints com calculo de orientação:" << kp.size() << std::endl;

  //removing blur applied in siftKPOrientation
  cv::cvtColor( img_norm, img_gray, CV_BGR2GRAY ); 

  //calculating keypoints description
  siftExecuteDescription( kp, img_gray );
  std::cout << "Size da lista de Keypoints  :" << kp.size() << std::endl;

  //printing keypoints and descriptions
  for( int i = 0; i < kp.size(); i++ )
    printKeypoint( kp[i] );
}