/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 Fondazione Istituto Italiano di Tecnologia (IIT)        *
 * All Rights Reserved.                                                       *
 *                                                                            *
 ******************************************************************************/

/**
 * @file Manager.h
 * @authors: Valentina Vasco <valentina.vasco@iit.it>
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <yarp/os/all.h>
#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>

#include <AssistiveRehab/skeleton.h>

#include "Manager.h"
#include "Processor.h"
#include "Metric.h"
#include "src/motionAnalyzer_IDL.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;

class Manager : public RFModule,
                public motionAnalyzer_IDL
{

    RpcClient opcPort;
    RpcServer rpcPort;
    BufferedPort<Bottle> scopePort;

    ResourceFinder *rf;

    int nmovements;

//    struct listEntry
//    {
//        string tag;
//        int id_joint;
//        int n_motion;
//        double min;
//        double max;
//    };
//    map<string, listEntry> motion_list;

    map<string,Metric*> motion_repertoire;

    vector<pair<string,Vector>> initial_keypoints;
//    vector<Vector> curr_keypoints;
    vector<pair<string,Vector>> curr_keypoints;
    SkeletonStd initial_skeleton;

    map<string, string> keypoints2conf;

    Vector elbowLeft_init;
    Vector elbowRight_init;
    Vector handLeft_init;
    Vector handRight_init;
    Vector head_init;
    Vector shoulderCenter_init;
    Vector shoulderLeft_init;
    Vector shoulderRight_init;
    Vector hipLeft_init;
    Vector hipRight_init;
    Vector kneeLeft_init;
    Vector kneeRight_init;
    Vector ankleLeft_init;
    Vector ankleRight_init;

    Vector elbowLeft;
    Vector elbowRight;
    Vector handLeft;
    Vector handRight;
    Vector head;
    Vector shoulderCenter;
    Vector shoulderLeft;
    Vector shoulderRight;
    Vector hipLeft;
    Vector hipRight;
    Vector kneeLeft;
    Vector kneeRight;
    Vector ankleLeft;
    Vector ankleRight;

//    Rom* crom;
//    vector<Rom*> rom;

    vector<Metric*> metrics;
    vector<Processor*> processors;

    void init();
    bool loadInitialConf();
    bool loadMotionList();
    void getKeyframes();
    void mapKeyframesToStandard(); //to map detected skeleton to standard skeleton
    bool attach(RpcServer &source);

public:

    bool configure(ResourceFinder &rf);
    bool close();
    double getPeriod();
    bool updateModule();

};

#endif