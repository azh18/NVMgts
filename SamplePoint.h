#ifndef SAMPLEPOINT_H
#define SAMPLEPOINT_H
#include "ConstDefine.h"

typedef struct SamplePoint{
        float lon;
        float lat;
        int time;
        int tid;
}SamplePoint;

SamplePoint generateSamplePoint(float lon1,float lat1,int time1,int tid1);
#endif // SAMPLEPOINT_H
