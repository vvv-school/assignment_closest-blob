#!/bin/bash

# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
# CopyPolicy: Released under the terms of the GNU GPL v3.0.

# Put here those instructions we need to execute before running the test

yarpdataplayer &
sleep 3
echo "load $ROBOT_CODE/datasets/dataDisparity" | yarp rpc /yarpdataplayer/rpc:i
