#pragma once
#define MAX_TRAJ_SIZE 20000
#define MAXLENGTH 256
//MAXGAP是最大轨迹内时间间隔，如果超过这个间隔应该被视为两条轨迹
#define MAXGAP 3600

#define EPSILON 10
#define MAXTHREAD 512
#define MAXJOBSNUM 10000

#include <stdio.h>
#include <string>
#include <sys/shm.h> // symphony header
#include <sys/socket.h> //socket header
#include <sys/sem.h>
#include <sys/types.h>
#define SMTYPE_NUM 8
#define SOCKETTYPE_NUM 8
#define SERVER_PORT 4081 // port no
#include <sys/time.h>
extern "C"{
#include "p_mmap.h"
}

//test:以cell为基础存储
#define _CELL_BASED_STORAGE
#define NVM_R(x) for(volatile int accnt=0;accnt<=5;accnt++){x;} //模拟NVM read latency
#define NVM_W(x) for(int accnt=0;accnt<=2;accnt++){x;} //模拟NVM write latency
//test:Similarity query based on naive grid，以定大小的grid来索引
//#define _SIMILARITY

typedef struct Point {
	float x;
	float y;
	uint32_t time;
	uint32_t tID;
}Point;

typedef struct SPoint {
	float x;
	float y;
	uint32_t tID;
}SPoint;

typedef struct SMDATA
{
    double dataDou[4];
    int type;
    char stringData[10000];
    bool flag[3];
    int dataInt[4];
} SMDATA;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};


