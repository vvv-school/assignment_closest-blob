/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
*/

#include <cmath>
#include <algorithm>

#include <rtf/yarp/YarpTestCase.h>
#include <rtf/dll/Plugin.h>

#include <yarp/os/Network.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Image.h>

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

using namespace RTF;

/**********************************************************************/
class TestAssignmentClosestBlob : public YarpTestCase
{
    yarp::os::BufferedPort<yarp::os::Bottle> port;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> > dispPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> > imagePort;
    yarp::os::RpcClient rpc;

public:
    /******************************************************************/
    TestAssignmentClosestBlob() :
        YarpTestCase("TestAssignmentClosestBlob")
    {
    }

    /******************************************************************/
    virtual ~TestAssignmentClosestBlob()
    {
    }

    /******************************************************************/
    virtual bool setup(yarp::os::Property& property)
    {
        port.open("/"+getName()+"/target:i");

        rpc.open("/"+getName()+"/rpc");

        imagePort.open("/"+getName()+"/image:i");

        dispPort.open("/"+getName()+"/disp:i");

        RTF_ASSERT_ERROR_IF(yarp::os::Network::connect("/closest-blob/target:o", port.getName() ), "Unable to connect to target!");
        RTF_ASSERT_ERROR_IF(yarp::os::Network::connect(rpc.getName(), "/yarpdataplayer/rpc:i"), "Unable to connect to target!");
        RTF_ASSERT_ERROR_IF(yarp::os::Network::connect("/icub/camcalib/left/out", imagePort.getName()), "Unable to connect to target!");
        RTF_ASSERT_ERROR_IF(yarp::os::Network::connect("/SFM/disp:o", dispPort.getName() ), "Unable to connect to target!");

        return true;
    }

    /******************************************************************/
    virtual void tearDown()
    {
        port.close();
        rpc.close();
        imagePort.close();
        dispPort.close();
    }

    /******************************************************************/
    virtual void run()
    {
        RTF_TEST_REPORT("sending PLAY to the dataset player");
        yarp::os::Bottle cmd, reply;
        cmd.addString("play");
        rpc.write(cmd,reply);

        RTF_TEST_REPORT(Asserter::format("Reply from datasetplayer is: %s\n", reply.toString().c_str()));

        yarp::os::ResourceFinder rf;
        rf.setDefaultContext("closest-blob");
        rf.setVerbose();
        rf.setDefaultContext(rf.getContext().c_str());

        int increment = 0;

        RTF_TEST_REPORT("Checking target position in the image");

        int frameArray[5] = {50, 118, 300, 340, 610};
        double successRate[5];

        for (int frameNum = 0 ; frameNum<800;)
        {
            yarp::sig::ImageOf<yarp::sig::PixelMono> *inDisp = dispPort.read(); //control image
            yarp::os::Bottle *pTarget=port.read(false);
            if (pTarget != NULL)
            {
                double tlx = pTarget->get(0).asList()->get(0).asDouble();
                double tly = pTarget->get(0).asList()->get(1).asDouble();
                double brx = pTarget->get(0).asList()->get(2).asDouble();
                double bry = pTarget->get(0).asList()->get(3).asDouble();

                int blobWidth = std::abs(brx - tlx);
                int blobHeight = std::abs(bry - tly);

                //RTF_TEST_REPORT(Asserter::format("GETTING BOUNDING BOX %lf %lf %lf %lf\n", tlx, tly, brx, bry));

                if (frameNum == frameArray[increment] +1 )
                {
                    yarp::sig::ImageOf<yarp::sig::PixelRgb> *inImage = imagePort.read();
                    cv::Mat inColour_cv = cv::cvarrToMat((IplImage *)inImage->getIplImage());

                    cv::Rect roi(tlx, tly, blobWidth, blobHeight);
                    cv::Mat image_roi(inColour_cv, roi);

                    //------------------------------------------------------------------------------------------ CALCULATE HISTOGRAMS

                    cv::Mat hsv;
                    cvtColor(image_roi, hsv, CV_RGB2HSV);
                    // Quantize the hue to 30 levels
                    // and the saturation to 32 levels
                    int hbins = 30, sbins = 32;
                    int histSize[] = {hbins, sbins};
                    // hue varies from 0 to 179, see cvtColor
                    float hranges[] = { 0, 180 };
                    // saturation varies from 0 (black-gray-white) to
                    // 255 (pure spectrum color)
                    float sranges[] = { 0, 256 };
                    const float* ranges[] = { hranges, sranges };
                    cv::MatND hist_user;
                    // we compute the histogram from the 0-th and 1-st channels
                    int channels[] = {0, 1};

                    calcHist( &hsv, 1, channels, cv::Mat(), // do not use mask
                             hist_user, 2, histSize, ranges,
                             true, // the histogram is uniform
                             false );

                    //------------------------------------------------------------------------------------------ LOAD HISTOGRAMS

                    std::ostringstream oss;
                    oss << "histogram_file" << increment+1 << ".yml";

                    std::string histogramFile = oss.str();
                    RTF_TEST_REPORT(Asserter::format("HISTOGRAM NAME IS: %s\n", histogramFile.c_str()));
                    //std::string histogramFile = "histogram_file" + std::to_string(increment+1) + ".yml";
                    std::string histogramPath = rf.findFile(histogramFile.c_str());

                    RTF_TEST_REPORT(Asserter::format("HISTOGRAM PATH IS: %s\n", histogramPath.c_str()));
                    // load file
                    cv::MatND hist_base;

                    cv::FileStorage fs( histogramPath.c_str(), cv::FileStorage::READ);
                    if (!fs.isOpened()) {std::cout << "unable to open file storage!" << std::endl; return;}
                    fs["my_histogram"] >> hist_base ;

                    fs.release();
                    increment++;

                    //std::string imageFile = "image_file" + std::to_string(increment+1) + ".png";
                    //cv::imwrite(imageFile, hsv);

                    //------------------------------------------------------------------------------------------ COMPARE HISTOGRAMS

                    for( int i = 0; i < 4; i++ )
                    {
                        int compare_method = i;
                        double comparison = compareHist( hist_base, hist_user, compare_method );
                        printf( " Method [%d] WITH : %f \n", i, comparison );
                    }

                    int compare_method = 0;
                    double comparison = compareHist( hist_base, hist_user, compare_method );

                    successRate[increment-1] = comparison;
                }
            }
            //RTF_TEST_REPORT(Asserter::format("FRAME NUMBER IS [%d]\n", frameNum));
            frameNum++;
        }

        double totalPrecentage = 0.0;
        for( int i = 0; i < increment; i++ )
        {
            RTF_TEST_REPORT(Asserter::format("SUCCESS RATE IS: %lf ", successRate[i]));
            totalPrecentage += successRate[i];
        }
        totalPrecentage = totalPrecentage/5;
        RTF_TEST_REPORT(Asserter::format("TOTAL SUCCESS PERCENTAGE IS: %lf\n", totalPrecentage));

        RTF_TEST_CHECK( totalPrecentage > 0.9 , "Checking passing of test");
    }
};

PREPARE_PLUGIN(TestAssignmentClosestBlob)
