#ifndef SCHEDULAR_H
#define SCHEDULAR_H
#include "ConstDefine.h"
#include "time.h"
#include "MBB.h"
#include "Grid.h"
#include "Trajectory.h"

extern int *stateData;

typedef struct Job{
    MBB queryMBR;
    int jobID = -1;
    bool completed=false;
    bool commited=false;
    CPURangeQueryResult *resultData=NULL;
    int resultNum=0;
    struct timeval queryTime;
    struct timeval finishTime;
}Job;

typedef Job TYPE;

//class myQueue
//{
//	TYPE* m_pData;
//	int m_nCount;
//	int m_nHead,m_nTail;
//
//public:
//	myQueue(int nCount)
//	{
//		m_nCount = nCount;
//		m_pData = new TYPE[nCount];//注意这里不是小括号，小扩号是调用构造函数
//		m_nHead = m_nTail = 0;
//
//	}
//	~myQueue(void)
//	{
//		delete []m_pData;
//	}
//	bool isEmpty()
//	{
//		return m_nHead == m_nTail;
//	}
//	bool isFull()//这里是难点
//	{
//		return (m_nTail+1)%m_nCount == m_nHead; //尾巴 加1 对总长度取 余数 如果与 头部 相等，则队列满，队列要预留出一个空间来判断是否满队列
//	}
//
//	void push(const TYPE& t)
//	{
//		if (isFull())
//		{
//			return;
//		}
//		m_pData[m_nTail++] = t;
//		m_nTail %= m_nCount;  //如果 尾巴 到了最后让他直接跑到 对头
//
//
//	}
//	bool pop()
//	{
//		if (isEmpty())
//		{
//			return false;
//		}
//		m_nHead++;
//		// t = m_pData[];
//		m_nTail %= m_nCount;
//		return true;
//
//	}
//
//	TYPE* front()
//	{
//		TYPE* t;
//		if (isEmpty())
//		{
//			return NULL;
//		}
//		t = &m_pData[m_nHead];
//		m_nTail %= m_nCount;
//		return t;
//	}
//
//};

typedef struct myQueue{
	TYPE* m_pData;
	int m_nCount;
	int m_nHead,m_nTail;
}myQueue;

int initMyQueue(myQueue *q, int nCount);

bool isEmpty(myQueue *q);
bool isFull(myQueue *q);//这里是难点

void push(myQueue *q,const TYPE& t);
bool pop(myQueue *q);

TYPE* front(myQueue *q);


typedef struct Schedular{
    myQueue* jobsBuffQueue;
    Grid *gridIndex;
    Trajectory* DB;
    int lastCompletedJob = -1;
    int jobIDMax = -1; //当前分配的最大jobID
    FILE *fp; //结果文件存放处
}Schedular;

int initSchedular(Schedular *sche, Grid *gridIndex, Trajectory *DB);
int runSchedular(Schedular *sche, Grid *gridIndex, Trajectory *DB);
int executeQueryInSchedular(Schedular *sche);
int loadJobs(Schedular *sche);
int writeResult(Schedular *sche);
bool destroySchedular(Schedular *sche);

//class Schedular
//{
//    public:
//        Schedular();
//        int init(Grid *gridIndex, Trajectory *DB);
//        int run(Grid *gridIndex, Trajectory *DB);
//        virtual ~Schedular();
//        int executeQuery();
//        int loadJobs();
//        int writeResult();
//
//        myQueue* jobsBuffQueue;
//        Grid *gridIndex;
//        Trajectory* DB;
//        int lastCompletedJob;
//        int jobIDMax; //当前分配的最大jobID
//        FILE *fp; //结果文件存放处
//
//    protected:
//
//    private:
//};

#endif // SCHEDULAR_H
