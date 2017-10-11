#include "Trajectory.h"


extern float calculateDistance(float LatA,float LonA,float LatB,float LonB);
extern int systemMode;

Trajectory generateTrajectory(int tid,std::string vid)
{
    Trajectory t;
    t.tid = tid;
    memcpy(t.vid,vid.c_str(),sizeof(char)*vid.length());
    t.length = 0;
    return t;
    //ctor
}

//往轨迹里添加采样点
//0:成功 1:超过最大数目 2:时间跨度太大，应计入下条轨迹 3:时间跨度不大，空间跨度太大，计算出来的速度大于180km/h~~50m/s，舍弃该点
__attribute__((optimize("O0")))
int addSamplePoints(Trajectory *traj, float lon,float lat,int time)
{
    //length指向的就是当前的idx，length超过最大值，则失败，返回1
    if(traj->length>=MAXLENGTH)
    {
        return 1;
    }
    if(traj->length>0)
    {
        if(time - traj->points[traj->length-1].time > MAXGAP)
        {
            return 2;
        }
        if((calculateDistance(lat,lon,traj->points[traj->length-1].lat,traj->points[traj->length-1].lon))/(time - traj->points[traj->length-1].time)>=50)
        {
            return 3;
        }
    }
    //经过检查可以加入这点到轨迹中
    if(systemMode==0){//in pure DRAM mode
	    traj->points[traj->length].lat = lat;
	    traj->points[traj->length].lon = lon;
	    traj->points[traj->length].tid = traj->tid;
	    traj->points[traj->length].time = time;
	    traj->length++;
    }
    else{ //in pure SCM and hybrid mode
	    NVM_W(traj->points[traj->length].lat = lat);
	    NVM_W(traj->points[traj->length].lon = lon);
	    NVM_W(traj->points[traj->length].tid = traj->tid);
	    NVM_W(traj->points[traj->length].time = time);
	    traj->length++;
    }
    return 0;
}


