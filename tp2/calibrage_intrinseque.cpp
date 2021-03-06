
#include <opencv2/opencv.hpp>
	#include <stdio.h>

#ifdef WIN32
	// unix to win porting
	#define	  __func__   __FUNCTION__
	#define	  __PRETTY_FUNCTION__   __FUNCTION__
	#define   snprintf   _snprintf
	#define	  sleep(n_seconds)   Sleep((n_seconds)*1000)
	#define   round(d)   floor((d) + 0.5)
#endif


// includes section

#include "Apicamera/src/cameraUVC.h"
#include "Calibration/src/chessboardcalibration.h"
#include "Apicamera/src/cameraOPENCV.h"
#include <iostream>
#include <fstream>

// functions section

bool readVideo( cv::VideoCapture *capture, cv::Mat *out)
{
	bool bOk = capture->grab();
	capture->retrieve(*out);

	return bOk;
}

void cameraUVC_getFrame( apicamera::CameraUVC *camera, cv::Mat *out1)
{
	cv::Mat(camera->get1Frame()).copyTo(*out1);
}

/**
 * Encapsulate ChessboardCalibration class to made it easier to use.
 */
class IntrinsicChessboardCalibrator
{
public:
	IntrinsicChessboardCalibrator( unsigned int _cbWidth, unsigned int _cbHeight, int _image_count, double _taille_carreau ,const char *_intrinsicFileName)
	{
		// initialize camera to store intrinsic parameters
		camera = new apicamera::CameraOPENCV();

		// initialize calibration
		calibrator = new ChessboardCalibration( camera, _image_count, _cbWidth, _cbHeight, _taille_carreau);
		intrinsicFileName = _intrinsicFileName;
		computingDone = false;
	}

	~IntrinsicChessboardCalibrator()
	{
		delete calibrator;
		delete camera;
	}

	void processFrame( const cv::Mat *inImg, cv::Mat *intrinsicA, cv::Mat *intrinsicK, cv::Mat *error, cv::Mat *outImg)
	{
		// do the computing only once
		if( ! computingDone )
		{
			// accumulate data 
			if( (! inImg) || (! inImg->data) )
				return;
			inImg->copyTo(*outImg);
			IplImage currentImage(*outImg);
			if( calibrator->add2DCornersSet(&currentImage) == -1 )
			{
				// there is enough data, do intrinsic calibration
				printf("Computing intrinsic parameters ...\n");
				camera->intrinsicError = calibrator->calcIntrinsicParameters();

				// save intrinsic parameters to file
				camera->saveIntrinsicParameters(intrinsicFileName);
				printf( "Intrinsic parameters saved to file '%s'.\n", intrinsicFileName);

				computingDone = true;
			}
		}

		// copy intrinsic parameters to outputs
		cv::Mat A( 3, 3, CV_32FC1, camera->intrinsicA);
		A.copyTo(*intrinsicA);
		cv::Mat K( 1, 4, CV_32FC1, camera->intrinsicK);
		K.copyTo(*intrinsicK);
		cv::Mat E( 1, 1, CV_32FC1, &(camera->intrinsicError));
		E.copyTo(*error);
	}
	
	bool getDone()
	{
		return computingDone;
	}

protected:
	// camera is used only to store/load/save intrinsic/extrinsic parameters
	apicamera::CameraOPENCV *camera;

	ChessboardCalibration *calibrator;
	const char *intrinsicFileName;
	bool computingDone;
};

void printMat( const cv::Mat *in, const char *printingMode, const char *outputMode, const char *outputFile)
{
	std::streambuf * buf;
	std::ofstream output_file;
	std::string output = outputMode;
	// Use the file to write the matrix.
	if( output == "file" )
	{
		output_file.open( outputFile );
		if( !output_file.is_open() )
		{
			std::cerr << "Unable to open \"" << outputFile << "\" to write matrice." << std::endl;
		}
		std::cout << "Writing in \"" << outputFile << "\" file." << std::endl;
		buf = output_file.rdbuf();
	}
	// Use default stdout to print the matrix.
	else if( output == "stdout" )
	{
		buf = std::cout.rdbuf();
	}
	std::ostream out( buf );

	std::string cv_format = printingMode;
	if( cv_format != "default" )
		out << format(*in, printingMode) << std::endl;
	else
		out << *in << std::endl;
}

void showImage( const char* windowName, const cv::Mat *in)
{
	if( in == NULL || ( in->cols == 0 && in->rows == 0 ) )
	{
		// invalid image, display empty image
		const int w = 200;
		const int h = 100;
		cv::Mat img = cv::Mat( h, w, CV_8UC3, cv::Scalar(0));
		cv::line( img, cv::Point( 0, 0), cv::Point( w-1, h-1), cv::Scalar(0,0,255), 2);
		cv::line( img, cv::Point( 0, h-1), cv::Point( w-1, 0), cv::Scalar(0,0,255), 2);
		cv::imshow( windowName, img);
		return;
	}
	else if( in->depth() == CV_32F  ||  in->depth() == CV_64F )
	{
		// float image must be normalized in [0,1] to be displayed
		cv::Mat img;
		cv::normalize( *in, img, 1.0, 0.0, cv::NORM_MINMAX);
		cv::imshow( windowName, img);
		return;
	}

	cv::imshow( windowName, *in);
}


int main(int argc, char *argv[])
{
	if(argc < 5)
		return -1; 
	unsigned int nbImage = atoi(argv[1]);
	unsigned int ligneMire = atoi(argv[2]);
	unsigned int colonneMire = atoi(argv[3]);
	double tailleCarreau = atof(argv[4]);
	// disable buffer on stdout to make 'printf' outputs
	// immediately displayed in GUI-console
	setbuf(stdout, NULL);

	// initializations section

	cv::VideoCapture capture_block2("/Shared/TP_VTDVR/LIRIS-VISION/Applications/Starling/resource/kth_walkingd1_person01.mpg");
	if( ! capture_block2.isOpened() )
	{
		printf( "Failed to open %s video file.\n", "/Shared/TP_VTDVR/LIRIS-VISION/Applications/Starling/resource/kth_walkingd1_person01.mpg");
		return -1;
	}
	// initialize camera UVC
	apicamera::OpenParameters openParam_block3_;
	openParam_block3_.width = 640;
	openParam_block3_.height = 480;
	openParam_block3_.fRate = 30;
	apicamera::CameraUVC camera_block3_;
	if( camera_block3_.open( 0, &openParam_block3_) != 0 )
	{
		printf( "failed to init UVC Camera. Exiting ...\n");
		exit(1);
	}
	IntrinsicChessboardCalibrator *intCal_block4 = new IntrinsicChessboardCalibrator( ligneMire, colonneMire, nbImage, tailleCarreau, "intrinsics.txt");

	int key = 0;
	bool paused = false;
	bool goOn = true;
	while( goOn && ! intCal_block4->getDone())
	{
		// processings section

		cv::Mat block2_out1;
		if( ! readVideo( &capture_block2, &block2_out1) )
		{
			printf("End of video file.\n");
			break;
		}
		cv::Mat block3_out1;
		cameraUVC_getFrame( &camera_block3_, &block3_out1);
		cv::Mat block4_out1;
		cv::Mat block4_out2;
		cv::Mat block4_out3;
		cv::Mat block4_out4;
		intCal_block4->processFrame( &block3_out1, &block4_out1, &block4_out2, &block4_out3, &block4_out4);
		/*printMat( &block4_out1, "default", "stdout", "");
		printMat( &block4_out2, "default", "stdout", "");
		printMat( &block4_out3, "default", "stdout", "");*/
		showImage( "block8 (ESC to stop, SPACE to pause)", &block4_out4);

		if( paused )
			key = cv::waitKey(0);
		else
			key = cv::waitKey(113);
		if( (key & 255) == ' ' )
			paused = ! paused;
		else if( key != -1 )
			goOn = false;
	}

	// cleanings section

	delete intCal_block4;

	return 0;
}

