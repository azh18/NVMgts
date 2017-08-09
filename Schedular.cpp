#include "Schedular.h"
#include <string>
#include <fstream>


//Queue:
int initMyQueue(myQueue *q, int nCount)
{
	q->m_nCount = nCount;
	//q->m_pData = new TYPE[nCount];//注意这里不是小括号，小扩号是调用构造函数
	q->m_pData = (TYPE*)p_malloc(6, sizeof(TYPE)*nCount);
	q->m_nHead = q->m_nTail = 0;
	return 0;
}

bool isEmpty(myQueue *q)
{
	return q->m_nHead == q->m_nTail;
}
bool isFull(myQueue *q)//这里是难点
{
	return (q->m_nTail+1)%q->m_nCount == q->m_nHead; //尾巴 加1 对总长度取 余数 如果与 头部 相等，则队列满，队列要预留出一个空间来判断是否满队列
}

void push(myQueue *q,const TYPE& t)
{
	if (isFull(q))
	{
		return;
	}
	q->m_pData[q->m_nTail++] = t;
	q->m_nTail %= q->m_nCount;  //如果 尾巴 到了最后让他直接跑到 对头


}
bool pop(myQueue *q)
{
	if (isEmpty(q))
	{
		return false;
	}
	q->m_nHead++;
	// t = m_pData[];
	q->m_nTail %= q->m_nCount;
	return true;

}

TYPE* front(myQueue *q)
{
	TYPE* t;
	if (isEmpty(q))
	{
		return NULL;
	}
	t = &q->m_pData[q->m_nHead];
	q->m_nTail %= q->m_nCount;
	return t;
}



//Schedular::Schedular()
//{
//    //ctor
//    this->lastCompletedJob = -1;
//}
//
//Schedular::~Schedular()
//{
//    //dtor
//}

int runSchedular(Schedular *sche, Grid *gridIndex, Trajectory *DB)
{
	sche->gridIndex = gridIndex;
	sche->DB = DB;
	if(sche->lastCompletedJob == -1)// have not been inited
	{
		//have not been inited, build queue in nvm
		initSchedular(sche, gridIndex, DB);
		std::ifstream in("queryList.txt",std::ios::in);
		char queryStr[1024];
		while(!in.eof())
		{
			in.getline(queryStr,1024);
			printf("%s",queryStr);
			if(queryStr[0] == '\0')
				break;
			// generate job
			Job newJob;
			newJob.jobID = sche->jobIDMax++;
			char *tokenPtr = strtok(queryStr, ",");
			newJob.queryMBR.xmin = atof(tokenPtr);
			newJob.queryMBR.xmax = atof(strtok(NULL, ","));
			newJob.queryMBR.ymin = atof(strtok(NULL, ","));
			newJob.queryMBR.ymax = atof(strtok(NULL, ","));
			newJob.queryTime = time(NULL);
			newJob.queryTimeClock = clock();
			//sche->jobsBuffQueue->push(newJob);
			push(sche->jobsBuffQueue,newJob);
		}
		in.close();
		sche->lastCompletedJob = 0;
	}
	else
		//处理队头，然后再正式接下来的处理
	{
		//schedular has been initialed, recover from nvm
		//remember to add the base addr
		char *baseAddr = (char*)p_get_base();
		sche->jobsBuffQueue = (myQueue*)p_get_malloc(5);
		sche->jobsBuffQueue->m_pData = (TYPE*)p_get_malloc(6);
		sche->fp = fopen("Performance.txt","a+");
		Job *pJob = front(sche->jobsBuffQueue);
		if(pJob==NULL)
		{
			cout << "all jobs have been handled, if you want to reload, enter R and then press enter." << endl;
			string press;
			cin >> press;
			if(press == "R")
			{
				std::ifstream in("queryList.txt",std::ios::in);
				char queryStr[1024];
				while(!in.eof())
				{
					in.getline(queryStr,1024);
					printf("%s",queryStr);
					if(queryStr[0] == '\0')
						break;
					// generate job
					Job newJob;
					newJob.jobID = sche->jobIDMax++;
					char *tokenPtr = strtok(queryStr, ",");
					newJob.queryMBR.xmin = atof(tokenPtr);
					newJob.queryMBR.xmax = atof(strtok(NULL, ","));
					newJob.queryMBR.ymin = atof(strtok(NULL, ","));
					newJob.queryMBR.ymax = atof(strtok(NULL, ","));
					newJob.queryTime = time(NULL);
					newJob.queryTimeClock = clock();
					//sche->jobsBuffQueue->push(newJob);
					push(sche->jobsBuffQueue,newJob);
				}
				in.close();
				sche->lastCompletedJob = 0;
				pJob = front(sche->jobsBuffQueue);
			}
			else
				return 0;
		}
		if((pJob->commited == false) && (pJob->completed == true))
		{
			//sche->writeResult();
			writeResult(sche);
			// 只有任务提交才算完成
			pJob->completeTime = time(NULL);
			pJob->completeTimeClock = clock();
			// 如果相差大于5s，说明重启过，用普通计时法就可以
			if(difftime(pJob->completeTime,pJob->queryTime)>5)
			{
				fprintf(sche->fp,"Query %d use time %f s\n",pJob->jobID,difftime(pJob->completeTime,pJob->queryTime));
				printf("Query %d use time %f s\n",pJob->jobID,difftime(pJob->completeTime,pJob->queryTime));
			}
			// 如果相差小于5s，说明可能可以用clock精确计时
			else
			{
				fprintf(sche->fp,"Query %d use time %f s\n",pJob->jobID,double(pJob->completeTimeClock-pJob->queryTimeClock)/1000);
				printf("Query %d use time %f s\n",pJob->jobID,double(pJob->completeTimeClock-pJob->queryTimeClock)/1000);
			}
			fflush(sche->fp);
			pJob->commited = true;
			pop(sche->jobsBuffQueue);
		}
		else if((pJob->commited == false) && (pJob->completed == false))
		{
			executeQueryInSchedular(sche);
			writeResult(sche);
			pJob->completed = true;
			pJob->completeTime = time(NULL);
			pJob->completeTimeClock = clock();
			// 如果相差大于5s，说明重启过，用普通计时法就可以
			if(difftime(pJob->completeTime,pJob->queryTime)>5)
			{
				fprintf(sche->fp,"Query %d use time %f s\n",pJob->jobID,difftime(pJob->completeTime,pJob->queryTime));
				printf("Query %d use time %f s\n",pJob->jobID,difftime(pJob->completeTime,pJob->queryTime));
			}
			// 如果相差小于5s，说明可能可以用clock精确计时
			else
			{
				fprintf(sche->fp,"Query %d use time %f s\n",pJob->jobID,double(pJob->completeTimeClock-pJob->queryTimeClock)/1000);
				printf("Query %d use time %f s\n",pJob->jobID,double(pJob->completeTimeClock-pJob->queryTimeClock)/1000);
			}
			fflush(sche->fp);
			pJob->commited = true;
			pop(sche->jobsBuffQueue);
		}
		else
		{
			pop(sche->jobsBuffQueue);
		}
	}
	while(!isEmpty(sche->jobsBuffQueue))
	{
		//execute the query
		Job *pJob = front(sche->jobsBuffQueue);
		executeQueryInSchedular(sche);
		pJob->completed = true;
		writeResult(sche);
		pJob->completeTime = time(NULL);
		pJob->completeTimeClock = clock();
		// 如果相差大于5s，说明重启过，用普通计时法就可以
		if(difftime(pJob->completeTime,pJob->queryTime)>5)
		{
			fprintf(sche->fp,"Query %d use time %f s\n",pJob->jobID,difftime(pJob->completeTime,pJob->queryTime));
			printf("Query %d use time %f s\n",pJob->jobID,difftime(pJob->completeTime,pJob->queryTime));
		}
		// 如果相差小于5s，说明可能可以用clock精确计时
		else
		{
			fprintf(sche->fp,"Query %d use time %f s\n",pJob->jobID,double(pJob->completeTimeClock-pJob->queryTimeClock)/1000);
			printf("Query %d use time %f s\n",pJob->jobID,double(pJob->completeTimeClock-pJob->queryTimeClock)/1000);
		}
		fflush(sche->fp);
		pJob->commited = true;
		pop(sche->jobsBuffQueue);
	}
	fclose(sche->fp);
}

//int Schedular::run(Grid *gridIndex, Trajectory *DB)
//{
//    // 如果系统初始启动，初始化队列，然后再正式接下来的处理
//
//}


int initSchedular(Schedular *sche, Grid *gridIndex, Trajectory *DB)
{
	//sche->jobsBuffQueue = (myQueue*)malloc(sizeof(myQueue));
	sche->jobsBuffQueue = (myQueue*)p_malloc(5, sizeof(myQueue));
	initMyQueue(sche->jobsBuffQueue,MAXJOBSNUM);
	sche->gridIndex = gridIndex;
	sche->DB = DB;
	sche->fp = fopen("Performance.txt","a+");
	sche->jobIDMax = 0;
	sche->lastCompletedJob = -1;
	return 0;
}

//int Schedular::init(Grid *gridIndex, Trajectory *DB)
//{
//    this->jobsBuffQueue = new myQueue(MAXJOBSNUM);
//    this->gridIndex = gridIndex;
//    this->DB = DB;
//    this->fp = fopen("Performance.txt","a+");
//    this->jobIDMax = 0;
//    this->lastCompletedJob = -1;
//}

//执行查询并将结果写入job队列内

int executeQueryInSchedular(Schedular *sche)
{
	Job *pJob = front(sche->jobsBuffQueue);
	//this->gridIndex->rangeQuery(pJob->queryMBR,pJob->resultData,&pJob->resultNum); //这里仅仅是写入了DRAM里，有malloc操作！
	rangeQuery(sche->gridIndex,pJob->queryMBR,pJob->resultData,&pJob->resultNum);
	return 0;
}



//int Schedular::executeQuery()
//{
//    Job *pJob = this->jobsBuffQueue->front();
//    //this->gridIndex->rangeQuery(pJob->queryMBR,pJob->resultData,&pJob->resultNum); //这里仅仅是写入了DRAM里，有malloc操作！
//    rangeQuery(this->gridIndex,pJob->queryMBR,pJob->resultData,&pJob->resultNum);
//}

//int Schedular::loadJobs()
//{
//
//}

int writeResult(Schedular *sche)
{
	Job *pJob = front(sche->jobsBuffQueue);
	int resultNum = pJob->resultNum;
	CPURangeQueryResult* pStart = pJob->resultData;
	CPURangeQueryResult *pLast=NULL;
	FILE *fp = fopen("RangeQueryResult.txt","a+");
	fprintf(fp,"Query ID:%d, Result Num:%d\n",pJob->jobID,pJob->resultNum);
	for(int i=0; i<=resultNum-1; i++)
	{
		if(pStart!=NULL)
		{
			fprintf(fp,"%f,%f,%d\n",pStart->x,pStart->y,pStart->traid);
			pLast = pStart;
			pStart = pStart->next;
			free(pLast);
		}
	}
	fclose(fp);
	return 0;
}

//
//-----------------------------
// how to permit atom?
//-----------------------------
//
bool destroySchedular(Schedular *sche)
{
	myQueue* tempQueue = sche->jobsBuffQueue;
	p_free(6);
	p_free(5);
	p_free(4);
	(*stateData) = 3;
}

//
//int Schedular::writeResult()
//{
//    Job *pJob = this->jobsBuffQueue->front();
//    int resultNum = pJob->resultNum;
//    CPURangeQueryResult* pStart = pJob->resultData;
//    CPURangeQueryResult *pLast=NULL;
//    FILE *fp = fopen("RangeQueryResult.txt","a+");
//    fprintf(fp,"Query ID:%d, Result Num:%d\n",pJob->jobID,pJob->resultNum);
//    for(int i=0;i<=resultNum-1;i++)
//    {
//        if(pStart!=NULL)
//        {
//            fprintf(fp,"%f,%f,%d\n",pStart->x,pStart->y,pStart->traid);
//            pLast = pStart;
//            pStart = pStart->next;
//            free(pLast);
//        }
//    }
//    fclose(fp);
//}
