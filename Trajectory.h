#ifndef TRAJECTORY_H
#define TRAJECTORY_H
#include "ConstDefine.h"
#include "SamplePoint.h"


// 轨迹类，记录该轨迹的采样点等信息，最长1024
typedef struct Trajectory{
        int tid;
        SamplePoint points[MAXLENGTH];
        int length = 0;
        char vid[40];
        int errCounter = 0;
        SamplePoint errPointBuff[10]; //备用
}Trajectory;

Trajectory generateTrajectory(int tid,std::string vid);
int addSamplePoints(Trajectory *traj,float lon,float lat,int time);


#endif // TRAJECTORY_H
