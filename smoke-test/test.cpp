/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
*/

#include <cstdlib>
#include <string>
#include <cmath>
#include <algorithm>

#include <yarp/robottestingframework/TestCase.h>
#include <robottestingframework/dll/Plugin.h>
#include <robottestingframework/TestAssert.h>


#include <yarp/os/Network.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Image.h>
#include <yarp/cv/Cv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

using namespace robottestingframework;

/**********************************************************************/
class TestAssignmentClosestBlob : public yarp::robottestingframework::TestCase
{

    yarp::os::BufferedPort<yarp::os::Bottle> port;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> > dispPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> > imagePort;
    yarp::os::RpcClient rpc;

public:
    /******************************************************************/
    TestAssignmentClosestBlob() :
        yarp::robottestingframework::TestCase("TestAssignmentClosestBlob")
    {
    }

    /******************************************************************/
    virtual ~TestAssignmentClosestBlob()
    {
    }

    /******************************************************************/
    virtual bool setup(yarp::os::Property& property)
    {
    
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("dataset"),"Dataset is unspecified!");
        std::string path = std::getenv("DATASETS_PATH");
        std::string dataset = property.find("dataset").asString();
        
        port.open("/"+getName()+"/target:i");

        rpc.open("/"+getName()+"/rpc");

        imagePort.open("/"+getName()+"/image:i");

        dispPort.open("/"+getName()+"/disp:i");

        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/closest-blob/target:o", port.getName() ), "Unable to connect to target!");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect(rpc.getName(), "/yarpdataplayer/rpc:i"), "Unable to connect to target!");
        
        yarp::os::Bottle cmd,rep;
        cmd.addString("load");
        cmd.addString(path+"/"+dataset);
        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("Loading %s ...",cmd.toString().c_str()));
        rpc.write(cmd,rep);
        
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/icub/camcalib/left/out", imagePort.getName()), "Unable to connect to target!");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/SFM/disp:o", dispPort.getName() ), "Unable to connect to target!");
      
        //connect ports to closest-blob
         
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/icub/camcalib/left/out", "/closest-blob/image:i"), "Unable to connect to target!");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/SFM/disp:o", "/closest-blob/disparity:i" ), "Unable to connect to target!");
        
        cmd.clear();
        cmd.addString("play");
        rpc.write(cmd,rep);

        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("Reply from datasetplayer is: %s\n", rep.toString().c_str()));

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
        
        yarp::os::ResourceFinder rf;
        rf.setDefaultContext("closest-blob");
        rf.setVerbose();
        rf.setDefaultContext(rf.getContext());

        int increment = 0;

        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Checking target position in the image");

        int frameArray[5] = {50, 118, 300, 340, 610};
        double successRate[5];

        for (int frameNum = 0 ; frameNum<800;)
        {
            yarp::sig::ImageOf<yarp::sig::PixelMono> *inDisp = dispPort.read(); //control image
            yarp::os::Bottle *pTarget=port.read(false);
            
            if (pTarget != NULL)
            {
                ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(pTarget->size() == 1, Asserter::format("GOT WRONG SIZE OF BOTTLE %d: %s", pTarget->size(), pTarget->toString().c_str()));
                ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(pTarget->get(0).asList()->size() == 4 , Asserter::format("GOT WRONG SIZE OF LISR %d: %s", pTarget->get(0).asList()->size(), pTarget->toString().c_str()));
                
                
                double tlx = pTarget->get(0).asList()->get(0).asDouble();
                double tly = pTarget->get(0).asList()->get(1).asDouble();
                double brx = pTarget->get(0).asList()->get(2).asDouble();
                double bry = pTarget->get(0).asList()->get(3).asDouble();

                int blobWidth = std::abs(brx - tlx);
                int blobHeight = std::abs(bry - tly);

                ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE((blobWidth > 0) && (blobHeight > 0), Asserter::format("GOT WRONG blob sizes %d: %d:", blobWidth,  blobHeight));

                //ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("GETTING BOUNDING BOX %lf %lf %lf %lf\n", tlx, tly, brx, bry));

                if (frameNum == frameArray[increment] +1 )
                {
                    yarp::sig::ImageOf<yarp::sig::PixelRgb> *inImage = imagePort.read();
                    cv::Mat inColour_cv = yarp::cv::toCvMat(*inImage);

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
                    ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("HISTOGRAM NAME IS: %s\n", histogramFile.c_str()));
                    //std::string histogramFile = "histogram_file" + std::to_string(increment+1) + ".yml";
                    std::string histogramPath = rf.findFile(histogramFile);

                    ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("HISTOGRAM PATH IS: %s\n", histogramPath.c_str()));
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
            //ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("FRAME NUMBER IS [%d]\n", frameNum));
            frameNum++;
        }

        double totalPercentage = 0.0;
        for( int i = 0; i < increment; i++ )
        {
            ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("SUCCESS RATE IS: %lf ", successRate[i]));
            totalPercentage += successRate[i];
        }
        totalPercentage = totalPercentage/5;
        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("TOTAL SUCCESS PERCENTAGE IS: %lf\n", totalPercentage));

        //ROBOTTESTINGFRAMEWORK_TEST_CHECK( totalPercentage > 0.9 , "Checking passing of test");
        
        int score = 0;
        
        if (totalPercentage > 0.95)
            score = 20;
        else if (totalPercentage > 0.90)
            score = 15;
        else if (totalPercentage > 0.80)
            score = 10;
        else if (totalPercentage > 0.70)
            score = 5;
        else if (totalPercentage > 0.60)
            score = 1;
        
        ROBOTTESTINGFRAMEWORK_TEST_CHECK(score>0,Asserter::format("Total score = %d",score));
    }
};

ROBOTTESTINGFRAMEWORK_PREPARE_PLUGIN(TestAssignmentClosestBlob)
