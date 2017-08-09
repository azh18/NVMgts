#include "SamplePoint.h"


SamplePoint generateSamplePoint(float lon1,float lat1,int time1,int tid1)
{
    //ctor
    SamplePoint p;
    p.lat = lat1;
    p.lon = lon1;
    p.time = time1;
    p.tid = tid1;
    return p;
}
