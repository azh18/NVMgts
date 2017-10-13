// Data_Structure_Test.cpp : 定义控制台应用程序的入口点。
//

#include <iostream>
#include <fstream>
#include <map>
#include "CalcInAxis.h"
#include "PreProcess.h"
#include "Trajectory.h"
#include <vector>
#include "Grid.h"
#include "Schedular.h"

extern "C" {
#include "p_mmap.h"
}



using namespace std;

map<string, tidLinkTable*> vidTotid;
map<string, tidLinkTable*>::iterator iter;

typedef struct SysInfo
{
    double ymin;
    double ymax;
    double xmin;
    double xmax;
    int maxTid;
} SysInfo;

//global
Trajectory* tradb=NULL;
Trajectory* tradbDRAM = NULL;//DRAM中的tradb的指针
Trajectory* tradbSCM = NULL; //NVM中的tradb的指针
int tradbNVMID = -1;
string baseDate = "2014-07-01";
SysInfo *sysInfo=NULL;
int *stateData = NULL;
Schedular *sche = NULL;
// 系统模式，DRAM&SCM
//0:纯DRAM
//1:SCM-DRAM
//2:纯SCM
int systemMode = 1;

// Semaphore & Shared Memory
int creat_sem(key_t key);
int set_semvalue(int semid);
int sem_p(int semid);
int sem_v(int semid);
int del_sem(int semid);
int socketMsgHandler(char* msg);

void* shm[SMTYPE_NUM] = {NULL};
SMDATA *shared[SMTYPE_NUM] = {NULL};
int shmid[SMTYPE_NUM];
int semid[SMTYPE_NUM];
int nowState = -1;// 0-7 is real state; -1 is not do anything..
int numTrajs=0,numPoints=0; // how many T and P system has now
int nowQueryID = -1;


/*
NVM  Table
-----------------------------------------------------------------------------
No                              note
1                               stateData, represent the state of original data
2                               tradb, all trajectory
3                               sysInfo, some current information of system now
4                               sche, Schedular data
5                               Queue represent the working queue
6                               array in the queue
*/

int cleanData()
{
    if(p_get_malloc(1)==NULL)
    {
        printf("Cleaning Data0...\n");
    }
    else
    {

        printf("Cleaning Data2...\n");
        stateData = (int*)p_get_malloc(1);
        //p_free(stateData);

        sche = (Schedular*)p_get_malloc(4);
        if(sche != NULL)
            destroySchedular(sche);

        printf("%d",(*stateData));
        tradb = (Trajectory*)p_get_malloc(2);

        if(*stateData > 1)
        {
            p_free(2);
            *stateData = 1;
            printf("freed tradb");
        }

        sysInfo = (SysInfo*)p_get_malloc(3);
        if(sysInfo != NULL)
            p_free(3);


        *stateData = 1;

    }
    p_clear();
    cout << "finishing deleting..." << endl;
    cout << "Clean Data Successfully!" << endl;
    return 0;
}


int main(int argc, char **argv)
{
    //printf("hellp");

    int indexIsExist = 0;
    int needReload_DRAM = 1;

    int WriteTrajectoryToFile(string outFileName, int numTra);
    //cout << "Hello world!" << endl;
    //cout << sizeof(Cell) << endl;
    //ifstream fin;
    //float lon1, lat1, lon2, lat2;
    //lat1 = +34.15265;
    //lon1 = +113.10221;
    //lat2 = +35.15221;
    //lon2 = +113.10222;
    //cout << calculateDistance(lat1, lon1, lat2, lon2) << endl;
    int sz;
    char c;
    //fgets(c,1,systemState);
    char config[100];
    // 系统模式设置（纯DRAM或混合内存模式）
    FILE* readState = fopen("config.ini","r+");
    if(readState!=NULL)
        //systemState
    {
        c = fgetc(readState);
        printf("%c\n",c);

        systemMode = atoi(&c);
        fclose(readState);
    }
    if(systemMode == 1)
    {
        p_init(200*1024*1024);
        printf("[init] System is now at SCM-DRAM mode.\n");
    }
    else if (systemMode == 0)
    {
        printf("[init] System is now at pure DRAM mode.\n");
    }
    else
    {
        p_init(200*1024*1024);
        printf("[init] System is now at pure SCM mode.\n");
    }

    // initial SM and semaphore
    for(int i=0; i<SMTYPE_NUM; i++)
    {
        shmid[i] = shmget((key_t)i,sizeof(SMDATA),0666|IPC_CREAT);
        if(shmid[i] == -1)
        {
            // fail
            printf("failed to create shared memory :%d",shmid[i]);
        }
        shm[i] = shmat(shmid[i],0,0);
        if(shm[i]==(void*)-1)
        {
            printf("shmat err\n");
        }
        shared[i] = (SMDATA*)shm[i];
        // initial semaphore
        semid[i] = creat_sem((key_t)i);
    }

//------------------------------------------------------------------------------------
// change memory mode
//------------------------------------------------------------------------------------
    if(argc == 3 && argv[1][0]== 'm' && argv[2][0] == 'd')
    {
        FILE* systemState = fopen("config.ini","w+");
        if(systemMode == 1)
            int ret = cleanData();
        systemMode = 0;
        fputc('0',systemState);
        fclose(systemState);
        printf("Switched to pure DRAM mode.\nPlease restart NVM_GTS\n");
        return 0;
    }

    if(argc == 3 && argv[1][0] == 'm' && argv[2][0] == 'h')
    {
        FILE *systemState = fopen("config.ini","w+");
        if(systemMode >= 1)
            int ret = cleanData();
        systemMode = 1;
        fputc('1',systemState);
        fclose(systemState);
        printf("Switched to SCM-DRAM mode.\nPlease restart NVM_GTS\n");
        return 0;
    }

    if(argc == 3 && argv[1][0] == 'm' && argv[2][0] == 's')
    {
        FILE *systemState = fopen("config.ini","w+");
        if(systemMode >= 1)
            int ret = cleanData();
        systemMode = 2;
        fputc('2',systemState);
        fclose(systemState);
        printf("Switched to pure SCM mode.\nPlease restart NVM_GTS\n");
        return 0;
    }

//------------------------------------------------------------------------------------
// clean data
//------------------------------------------------------------------------------------
    //clean the data but failed
    if(argc == 2 && argv[1][0]== 'c')
    {
        //*stateData = 3;
        int ret = cleanData();
        if(ret == 0)
            return 0;
        else
            printf("Something wrong with cleanning procedure.\n");
        return 1;
    }


    /*
    Load Data...
    If system is down, recover the pointer from NVM
    -----------------------------------------------------------------------------------
    */

        string filename = "SH_4_3.txt";
        if((systemMode == 0)&&(needReload_DRAM ==1)) //纯内存模式
        {
            printf("Allocating DRAM...\n");
            tradbDRAM = (Trajectory*)malloc(sizeof(Trajectory)*MAX_TRAJ_SIZE);
            memset(tradbDRAM,0,sizeof(Trajectory)*MAX_TRAJ_SIZE);
            printf("Loading Data...\n");
            PreProcess pp;
            pp.init(filename, "dataout.txt",tradbDRAM);
            tradb = tradbDRAM;
            sysInfo = (SysInfo*)malloc(sizeof(SysInfo));
            sysInfo->xmin = pp.xmin;
            sysInfo->xmax = pp.xmax;
            sysInfo->ymin = pp.ymin;
            sysInfo->ymax = pp.ymax;
            sysInfo->maxTid = pp.maxTid;
            needReload_DRAM = 0;
        }
        else //混合内存模式
        {
            if((p_get_malloc(1)==NULL)||(argc == 2 && argv[1][0]== 'r'))
            {
                printf("Data not loaded, Loading Data...\n");

//        stateData = (int*)p_malloc(sizeof(int));
//        p_bind(1,stateData,sizeof(int));
                stateData = (int*)p_malloc(1,sizeof(int));
                //p_bind(1,stateData,sizeof(int));
                (*stateData) = 1; //build the stateData but not load in data
                printf("Allocating NVM...\n");
                tradbSCM = (Trajectory*)p_malloc(2, sizeof(Trajectory)*MAX_TRAJ_SIZE);
                memset(tradbSCM,0,sizeof(Trajectory)*MAX_TRAJ_SIZE);
                printf("malloc:%p\n",tradbSCM);
                printf("Loading Data...\n");
                (*stateData) = 2; //NVM allocated
                PreProcess pp;
                pp.init(filename, "dataout.txt",tradbSCM);
                sysInfo = (SysInfo*)p_malloc(3, sizeof(SysInfo));
                sysInfo->xmin = pp.xmin;
                sysInfo->xmax = pp.xmax;
                sysInfo->ymin = pp.ymin;
                sysInfo->ymax = pp.ymax;
                sysInfo->maxTid = pp.maxTid;
                (*stateData) = 3; //build data finished
                printf("Load Data finished\n");
            }
            else
            {
                stateData = (int*)p_get_malloc(1);
                char *base = (char*)p_get_base();
                //check if the process is finished
                if((*stateData)==1)
                {
                    // malloc and load data again
                    printf("Data not loaded, Loading Data2...\n");
                    tradbSCM = (Trajectory*)p_malloc(2, sizeof(Trajectory)*MAX_TRAJ_SIZE);
                    memset(tradbSCM,0,sizeof(Trajectory)*MAX_TRAJ_SIZE);
                    printf("malloc: %p\n", tradbSCM);
                    if(tradb == NULL)
                    {
                        printf("error allocate traDB\n");
                    }
                    (*stateData) = 2; //NVM allocated
                    PreProcess pp;
                    pp.init(filename, "dataout.txt",tradbSCM);
                    sysInfo = (SysInfo*)p_malloc(3, sizeof(SysInfo));
                    sysInfo->xmin = pp.xmin;
                    sysInfo->xmax = pp.xmax;
                    sysInfo->ymin = pp.ymin;
                    sysInfo->ymax = pp.ymax;
                    sysInfo->maxTid = pp.maxTid;
                    (*stateData) = 3; //build data finished
                    printf("Load Data finished\n");
                }
                else if((*stateData)==2)
                {
                    // load data again
                    printf("Data not loaded, Loading Data3...\n");
                    tradbSCM= (Trajectory*)p_get_malloc(2);
                    memset(tradbSCM,0,sizeof(Trajectory)*MAX_TRAJ_SIZE);
                    PreProcess pp;
                    pp.init(filename, "dataout.txt",tradbSCM);
                    sysInfo = (SysInfo*)p_malloc(3, sizeof(SysInfo));
                    sysInfo->xmin = pp.xmin;
                    sysInfo->xmax = pp.xmax;
                    sysInfo->ymin = pp.ymin;
                    sysInfo->ymax = pp.ymax;
                    sysInfo->maxTid = pp.maxTid;
                    (*stateData) = 3;
                    printf("Load Data finished\n");
                }
                else
                {
                    // stateData>=3, load finished, only need bind and query
                    //printf("Data loaded.\n");
                    tradbSCM = (Trajectory*)p_get_malloc(2);
                    sysInfo = (SysInfo*)p_get_malloc(3);
                    //printf("location:%f,%f;time:%d;Tid:%d\n",tradbSCM[3].points[2].lat,tradbSCM[3].points[2].lon,tradbSCM[3].points[2].time,tradbSCM[3].points[2].tid);
                }

            }
            tradb = tradbSCM;
        }
        cout << "read trajectory success!" << endl;

    /*
    Build Grid Index... (Auto Run After No Index)
    -----------------------------------------------------------------------------------
    */
    //cout << WriteTrajectoryToFile("dataOut.txt", sysInfo->maxTid) << endl;

    Grid *g =NULL;
    if(indexIsExist == 0)
    {
        cout << "Start building cell index" << endl;
        //Grid* g = new Grid(MBB(sysInfo->xmin, sysInfo->ymin, sysInfo->xmax, sysInfo->ymax), 0.003);
        g = new Grid();
        initGrid(g,MBB(sysInfo->xmin, sysInfo->ymin, sysInfo->xmax, sysInfo->ymax), 0.003);
        //g->addDatasetToGrid(tradb, sysInfo->maxTid);
        addDatasetToGrid(g,tradb, sysInfo->maxTid);
        int count = 0;
        for (int i = 0; i <= g->cellnum - 1; i++)
        {
            if (g->cellPtr[i].subTraNum == 0)
                count++;
        }
        //cout << "zero num:" << count << "total" << g->cellnum << endl;
        //int temp[7] = { 553,554,555,556,557,558,559 };
        //int sizetemp = 7;
        //g->writeCellsToFile(temp, sizetemp, "111.txt");
        numTrajs = sysInfo->maxTid;
        numPoints = g->totalPointNum;
        // output new system status
        //.......
        // output new system status
        cout << "Building index successful"<<endl;
    }

    if(argc == 2 && argv[1][0]== 's')
    {
        //单独用户输入query测试
        CPURangeQueryResult* resultTable=(CPURangeQueryResult*)malloc(sizeof(CPURangeQueryResult));
        int RangeQueryResultSize = 0;

        MBB SHMbb = MBB(121.36, 31.10, 121.56, 31.36);
        MBB *queryMBB = new MBB[1000];
        SHMbb.randomGenerateInnerMBB(queryMBB,1000);
        int qcnt=0;
        for (int i=0; i<=1; i++)
        {
            printf("RQ(%f,%f,%f,%f)\n",queryMBB[i].xmin,queryMBB[i].xmax,queryMBB[i].ymin,queryMBB[i].ymax);
            rangeQuery(g,queryMBB[i],resultTable,&RangeQueryResultSize);
            printf("Query ID: %d\n", qcnt++);
            printf("Result Num: %d", RangeQueryResultSize);
            CPURangeQueryResult* first = resultTable->next;
            for(int tt=0; tt<=RangeQueryResultSize-1; tt++)
            {
                printf("[%d](%f,%f)\t",first->traid, first->x, first->y);
                first = first->next;
            }
            printf("\n");
        }
    }

    if(argc == 2 && argv[1][0]== 'f')
    {
        /*
        Load Schedular... If system is down, restart after data and index are all fine
        -----------------------------------------------------------------------------------
        */
        if(systemMode==0)
        {
            sche = (Schedular*)malloc(sizeof(Schedular));
            sche->lastCompletedJob = -1;
            cout << "begin running schedular..." << endl;
            runSchedular(sche,g,tradb);
        }
        else
        {
            if(*stateData==3)
            {
                //stateData==3 means we have not build a schedular, haven't run
                sche = (Schedular*)p_malloc(4, sizeof(Schedular));
                *stateData = 4;
                sche->lastCompletedJob = -1;
                cout << "begin running schedular..." << endl;
                runSchedular(sche,g,tradb);
            }
            else
            {
                //stateData>=4 means schedular has been in NVM
                sche = (Schedular*)p_get_malloc(4);
                cout << "begin continuing schedular..." << endl;
                runSchedular(sche,g,tradb);
            }
        }
    }
//    Schedular schedular;
//    runSchedular(&schedular,g,tradb);
    //schedular.run(g,tradb);


    //g->rangeQuery(queryMbb, resultTable, &RangeQueryResultSize);
    //rangeQuery(g,queryMbb, resultTable, &RangeQueryResultSize);


//
//	ofstream ftest;
//	ftest.open("ftest.txt", ios_base::out);
//	ftest << g->totalPointNum << endl;
//	for (int i = 0; i <= g->cellnum - 1; i++) {
//		ftest << g->cellPtr[i].totalPointNum << ",";
//	}
//	ftest << endl;


    cout << "build cell index success!" << endl;
    getchar();
    getchar();
    getchar();

    return 0;
}

int WriteTrajectoryToFile(string outFileName,int numTra)
{
    ofstream fout;
    fout.open(outFileName, ios_base::out);
    for (int i = 1; i <= numTra; i++)
    {
        fout << i << ": ";
        for (int j = 0; j <= tradb[i].length - 1; j++)
        {
            fout << tradb[i].points[j].lon << "," << tradb[i].points[j].lat << ";";
        }
        fout << endl;
    }
    fout.close();
    return 1;
}

int creat_sem(key_t key)
{
    int semid = 0;

    semid = semget(key, 1, IPC_CREAT|0666);
    if(semid == -1)
    {
        printf("%s : semid = -1!\n",__func__);
        return -1;
    }

    return semid;

}

int set_semvalue(int semid)
{
    union semun sem_arg;
    sem_arg.val = 1;

    if(semctl(semid, 0, SETVAL, sem_arg) == -1)
    {
        printf("%s : can't set value for sem!\n",__func__);
        return -1;
    }
    return 0;
}

int sem_p(int semid)
{
    struct sembuf sem_arg;
    sem_arg.sem_num = 0;
    sem_arg.sem_op = -1;
    sem_arg.sem_flg = SEM_UNDO;

    if(semop(semid, & sem_arg, 1) == -1)
    {
        printf("%s : can't do the sem_p!\n",__func__);
        return -1;
    }
    return 0;
}

int sem_v(int semid)
{
    struct sembuf sem_arg;
    sem_arg.sem_num = 0;
    sem_arg.sem_op = 1;
    sem_arg.sem_flg = SEM_UNDO;

    if(semop(semid, & sem_arg, 1) == -1)
    {
        printf("%s : can't do the sem_v!\n",__func__);
        return -1;
    }

    return 0;
}

int del_sem(int semid)
{
    if(semctl(semid, 0, IPC_RMID) == -1)
    {
        printf("%s : can't rm the sem!\n",__func__);
        return -1;
    }
    return 0;
}

int renewSystemState(int trajN, int pointN, int queryIDRunning, int runningType)
{
    numTrajs = trajN;
    numPoints = pointN;
    nowState = runningType;
    nowQueryID = queryIDRunning;
    if(sem_p(semid[4]))
    {
        printf("sem_p fail.\n");
    }
    shared[4]->dataInt[0] = numTrajs;
    shared[4]->dataInt[1] = numPoints;
    shared[4]->dataInt[2] = nowQueryID;
    shared[4]->dataInt[3] = nowState;
    shared[4]->flag[0] = true;
    switch(systemMode)
    {
        case 0:
        {
            shared[4]->stringData[0] = 'd';
            break;
        }
        case 1:
        {
            shared[4]->stringData[0] = 'h';
            break;
        }
        case 2:
        {
            shared[4]->stringData[0] = 's';
            break;
        }
        default:
        {
            shared[4]->stringData[0] = 'n'; //unknown
            break;
        }
    }
    if(sem_v(semid[4]))
    {
        printf("sem_v fail.\n");
    }
    return 0;
}

// updateSM(SMDATA smdata, )
// It is meaningless to write a unified function to update SM because it does not simplify the code.
