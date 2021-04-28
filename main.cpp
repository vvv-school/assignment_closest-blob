/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff
 * email:  vadim.tikhanoff@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

//dataset https://cp.sync.com/dl/2e475c930/gz26kem4-7fph5kpr-wyfxpdqu-36jrmkvn

#include <cstdlib>

#include <yarp/os/BufferedPort.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Network.h>
#include <yarp/os/Log.h>
#include <yarp/os/Time.h>
#include <yarp/os/LogStream.h>
#include <yarp/sig/Image.h>
#include <yarp/os/RpcClient.h>
#include <yarp/cv/Cv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

#include "closestBlob_IDL.h"

/********************************************************/
class Processing : public yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> >
{
    std::string moduleName;

    yarp::os::RpcServer handlerPort;

    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >   inPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >   outPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >   cropOutPort;
    yarp::os::BufferedPort<yarp::os::Bottle>  targetPort;

    yarp::os::RpcClient rpc;

    cv::Mat inColour_cv;
    cv::Mat disp;

public:
    /********************************************************/

    Processing( const std::string &moduleName )
    {
        this->moduleName = moduleName;
    }

    /********************************************************/
    ~Processing()
    {

    };

    /********************************************************/
    bool open(){

        this->useCallback();

        BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> >::open( "/" + moduleName + "/disparity:i" );
        inPort.open("/"+ moduleName + "/image:i");
        outPort.open("/"+ moduleName + "/image:o");
        cropOutPort.open("/" + moduleName + "/crop:o");
        targetPort.open("/"+ moduleName + "/target:o");

        return true;
    }

    /********************************************************/
    void close()
    {
        inPort.close();
        outPort.close();
        targetPort.close();
        cropOutPort.close();
        BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> >::close();
    }

    /********************************************************/
    void interrupt()
    {
        BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> >::interrupt();
    }

    /********************************************************/
    void onRead( yarp::sig::ImageOf<yarp::sig::PixelMono> &dispImage )
    {
        yarp::sig::ImageOf<yarp::sig::PixelRgb> &outImage  = outPort.prepare();
        yarp::sig::ImageOf<yarp::sig::PixelRgb> &cropOutImage  = cropOutPort.prepare();
        yarp::os::Bottle &outTargets = targetPort.prepare();
        
        yarp::sig::ImageOf<yarp::sig::PixelRgb> *inImage = inPort.read();

        outImage.resize(dispImage.width(), dispImage.height());
        cropOutImage.resize(dispImage.width(), dispImage.height());
        
        outImage.zero();
        cropOutImage.zero();
        
        inColour_cv = yarp::cv::toCvMat(*inImage);  // prepare the image ports and targets
        disp = yarp::cv::toCvMat(dispImage);
        
        //FILL IN THE CODE
        
        // Apply image processing techniques on the disparity image to smooth things out 
    
        // Apply some threshold on the image to remove background:
        // have a look at cv::threshold function

        // Find the max value and its position

        //....

        //Find the contour of the closest objects with moments and mass center
        //
        
        //....

        // optional hint: you could use pointPolygonTest and the previous maxvalue location to compare with all contours found and get the actual brightest one

        //....

        // Use the result of pointPolygonTest or your own technique as the closest contour to:
        // 1 - draw it on the disparity image
        // 2 - create a cropped image containing the rgb roi
        // 3 - fill in a yarp bottle with the bounding box

        //be aware that the expected Bottle should be a list containing:
        // (tl.x tl.y br.x br.y)
        //where tl is top left and br - bottom right

        cvtColor(disp, disp, CV_GRAY2RGB);

        outTargets.clear();
        
        if (outTargets.size() >0 )
            targetPort.write();

        outImage=yarp::cv::fromCvMat<yarp::sig::PixelRgb>(disp);
        outPort.write();

        cropOutImage=yarp::cv::fromCvMat<yarp::sig::PixelRgb>(inColour_cv);
        cropOutPort.write();
    }
};

/********************************************************/
class Module : public yarp::os::RFModule, public closestBlob_IDL
{
    yarp::os::ResourceFinder    *rf;
    yarp::os::RpcServer         rpcPort;

    Processing                  *processing;
    friend class                processing;

    bool                        closing;

    /********************************************************/
    bool attach(yarp::os::RpcServer &source)
    {
        return this->yarp().attachAsServer(source);
    }

public:

    /********************************************************/
    bool configure(yarp::os::ResourceFinder &rf)
    {
        this->rf=&rf;
        std::string moduleName = rf.check("name", yarp::os::Value("closest-blob"), "module name (string)").asString();
        setName(moduleName.c_str());

        rpcPort.open("/"+getName("/rpc"));

        closing = false;

        processing = new Processing( moduleName );

        /* now start the thread to do the work */
        processing->open();

        attach(rpcPort);

        return true;
    }

    /**********************************************************/
    bool close()
    {
        processing->interrupt();
        processing->close();
        delete processing;
        return true;
    }

    /**********************************************************/
    bool quit(){
        closing = true;
        return true;
    }

    /********************************************************/
    double getPeriod()
    {
        return 0.1;
    }

    /********************************************************/
    bool updateModule()
    {
        return !closing;
    }
};

/********************************************************/
int main(int argc, char *argv[])
{
    yarp::os::Network::init();

    yarp::os::Network yarp;
    if (!yarp.checkNetwork())
    {
        yError("YARP server not available!");
        return EXIT_FAILURE;
    }

    Module module;
    yarp::os::ResourceFinder rf;

    rf.setVerbose();
    rf.configure(argc,argv);

    return module.runModule(rf);
}
