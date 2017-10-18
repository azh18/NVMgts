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
    int datasetInNVM = 0;
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
int renewSystemState(int trajN, int pointN, int queryIDRunning, int runningType);
double getTimeus();

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
//        sysInfo->datasetInNVM = -1;
//        sysInfo->maxTid = 0;
    }
    p_clear();
    vidTotid.clear();

    cout << "finishing deleting..." << endl;
    cout << "Clean Data Successfully!" << endl;
    return 0;
}


int main(int argc, char **argv)
{
    //printf("hellp");

    int indexIsExist = 0;
    int needReload_DRAM = 0; // to control whether in DRAM mode the data need to be reload, by default yes
    int needReLoadData_NVM = 0; // to control whether in NVM data need to be reload. by default the data need to be loaded.
    int datasetNowID = -1;
    int needDatasetUsedByRecovery = 0; // store the needed dataset. Only be used in recovery.
    Grid *g =NULL;
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
        p_init(200*1024*1024);
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
        shmid[i] = shmget((key_t)i+1,sizeof(SMDATA),0666|IPC_CREAT);
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
    printf("stage 1\n");
    //if in NVM mode recovery sys status
    if(sem_p(semid[4]))
    {
        printf("sem_p fail.\n");
    }
    if(systemMode >=1)
    {
        nowState = shared[4]->dataInt[3];
        numTrajs = shared[4]->dataInt[0];
        numPoints = shared[4]->dataInt[1];
        nowQueryID = shared[4]->dataInt[2];
    }
    if(sem_v(semid[4]))
    {
        printf("sem_v fail.\n");
    }
    printf("stage 2\n");
    while(1)
    {

//------------------------------------------------------------------------------------
// change memory mode
//------------------------------------------------------------------------------------
        printf("test change memory mode...\n");
        if(sem_p(semid[3]))
        {
            printf("sem_p fail.\n");
        }

        if((shared[3]->flag[0]) || (argc == 3))
        {
            renewSystemState(0,0,-1,3);
            if((shared[3]->stringData[0]=='d')||(argc == 3 && argv[1][0]== 'm' && argv[2][0] == 'd'))
            {
                FILE* systemState = fopen("config.ini","w+");
                if(systemMode == 1)
                    int ret = cleanData();
                systemMode = 0;
                fputc('0',systemState);
                fclose(systemState);
                shared[3]->flag[0] = false;
                shared[3]->flag[1] = true;
                renewSystemState(-1,-1,-1,-1);
                printf("Switched to pure DRAM mode.\nPlease restart NVM_GTS\n");
                needReload_DRAM = 0;
                datasetNowID = 0;
                indexIsExist = 0;
            }

            if((shared[3]->stringData[0]=='h')||(argc == 3 && argv[1][0] == 'm' && argv[2][0] == 'h'))
            {
                FILE *systemState = fopen("config.ini","w+");
                int ret = cleanData();
                systemMode = 1;
                fputc('1',systemState);
                fclose(systemState);
                shared[3]->flag[0] = false;
                shared[3]->flag[1] = true;
                renewSystemState(-1,-1,-1,-1);
                printf("Switched to SCM-DRAM mode.\nPlease restart NVM_GTS\n");
                needReLoadData_NVM = 0;
                datasetNowID = 0;
                indexIsExist = 0;
            }

            if((shared[3]->stringData[0]=='s')||(argc == 3 && argv[1][0] == 'm' && argv[2][0] == 's'))
            {
                FILE *systemState = fopen("config.ini","w+");
                int ret = cleanData();
                systemMode = 2;
                fputc('2',systemState);
                fclose(systemState);
                shared[3]->flag[0] = false;
                shared[3]->flag[1] = true;
                renewSystemState(-1,-1,-1,-1);
                printf("Switched to pure SCM mode.\nPlease restart NVM_GTS\n");
                needReLoadData_NVM = 0;
                datasetNowID = 0;
                indexIsExist = 0;
            }
        }
        printf("test change memory mode...\n");
        if(sem_v(semid[3]))
        {
            printf("sem_v fail.\n");
        }
//------------------------------------------------------------------------------------
// clean data
//------------------------------------------------------------------------------------
        //clean the data but failed
        printf("test cleaning data...");
        if(sem_p(semid[5]))
        {
            printf("sem_p fail.\n");
        }

        if((shared[5]->flag[1])||(argc == 2 && argv[1][0]== 'c'))
        {
            renewSystemState(0,0,-1,5);
            //*stateData = 3;
            int ret = cleanData();
            printf("cleancleancealn..............\n");
            if(ret == 0)
                //return 0;
            {
                shared[5]->flag[1] = false;
                shared[5]->flag[0] = true;
                renewSystemState(-1,-1,-1,-1);
                datasetNowID = 0;
                needReLoadData_NVM = 0;
                needReload_DRAM = 0;
            }
            else
            {
                shared[5]->flag[1] = true;
                shared[5]->flag[0] = false;
                renewSystemState(-1,-1,-1,5);
                printf("Something wrong with cleanning procedure.\n");
                sleep(2);
            }
            if(argv[1][0]== 'c')
                return 0;
            //return 1;
        }
        if(sem_v(semid[5]))
        {
            printf("sem_v fail.\n");
        }

        /*
        Load Data...
        If system is down, recover the pointer from NVM
        -----------------------------------------------------------------------------------
        */
        double startLoadTime;
        double endLoadTime;
        if(sem_p(semid[0]))
        {
            printf("sem_p fail.\n");
        }
        if(shared[0]->flag[1])
        {
            startLoadTime = getTimeus();
            shared[0]->dataDou[0] = startLoadTime;
            if(datasetNowID != shared[0]->dataInt[2])
            {
                needReload_DRAM = shared[0]->dataInt[2];
                needReLoadData_NVM = shared[0]->dataInt[2];
                renewSystemState(0,0,-1,0);
            }
            renewSystemState(-1,-1,-1,0);
            shared[0]->flag[1] = false;
        }
        if((systemMode == 0)&&(datasetNowID != shared[0]->dataInt[2])){
            needReload_DRAM = shared[0]->dataInt[2];
        }
        else
        {
            needDatasetUsedByRecovery = shared[0]->dataInt[2];
        }

        if(sem_v(semid[0]))
        {
            printf("sem_v fail.\n");
        }

        //string filename = "SH_4_3.txt";
        if((systemMode == 0)&&(needReload_DRAM >= 1)) //纯内存模式
        {
            string filename = "SH_4_"+ to_string(needReload_DRAM) + ".txt";
            printf("Allocating DRAM...\n");
            tradbDRAM = (Trajectory*)malloc(sizeof(Trajectory)*MAX_TRAJ_SIZE);
            vidTotid.clear();
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
            datasetNowID = needReload_DRAM;
            needReload_DRAM = 0;
            indexIsExist = 0;
        }
        else if(systemMode >= 1)//混合内存模式
        {
            if((p_get_malloc(1)==NULL)||(argc == 2 && argv[1][0]== 'r') ||(needReLoadData_NVM >= 1)) // no data or need reload
            {
                int datasetID;
                string filename;
                if(needReLoadData_NVM>=1)
                {
                    datasetID = needReLoadData_NVM;
                    filename = "SH_4_"+ to_string(needReLoadData_NVM) + ".txt";
                    printf("Data not loaded, Loading Data...\n");
                    cleanData();
                }
                else
                {
                    datasetID = needDatasetUsedByRecovery;
                    filename = "SH_4_"+ to_string(needDatasetUsedByRecovery) + ".txt";
                    printf("Data not loaded, Loading Data...\n");
                    vidTotid.clear();
                }

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
                sysInfo->datasetInNVM = datasetID;
                (*stateData) = 3; //build data finished
                printf("Load Data finished\n");
                datasetNowID = datasetID;
                needReLoadData_NVM = 0;
                indexIsExist = 0;
            }
            else //no reload
            {
                stateData = (int*)p_get_malloc(1);
                char *base = (char*)p_get_base();
                //check if the process is finished
                if((*stateData)>=3)
                {
                    sysInfo = (SysInfo*)p_get_malloc(3);
                    datasetNowID = sysInfo->datasetInNVM;
                }
                if((*stateData)==1)
                {
                    string filename = "SH_4_"+ to_string(needDatasetUsedByRecovery) + ".txt";
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
                    sysInfo->datasetInNVM = needDatasetUsedByRecovery;
                    (*stateData) = 3; //build data finished
                    printf("Load Data finished\n");
                    datasetNowID = sysInfo->datasetInNVM;
                    needReLoadData_NVM = 0;

                    indexIsExist = 0;
                }
                else if((*stateData)==2)
                {
                    string filename = "SH_4_"+ to_string(needDatasetUsedByRecovery) + ".txt";
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
                    sysInfo->datasetInNVM = needDatasetUsedByRecovery;
                    (*stateData) = 3;
                    printf("Load Data finished\n");
                    needReLoadData_NVM = 0;
                    datasetNowID = sysInfo->datasetInNVM;
                    indexIsExist = 0;
                }
                else //>=3
                {
                    string filename = "SH_4_"+ to_string(datasetNowID) + ".txt";
                    // stateData>=3, load finished, only need bind and query
                    //printf("Data loaded.\n");
                    tradbSCM = (Trajectory*)p_get_malloc(2);
                    sysInfo = (SysInfo*)p_get_malloc(3);
                    //printf("location:%f,%f;time:%d;Tid:%d\n",tradbSCM[3].points[2].lat,tradbSCM[3].points[2].lon,tradbSCM[3].points[2].time,tradbSCM[3].points[2].tid);
                    needReLoadData_NVM = 0;
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


        if((indexIsExist == 0) && (datasetNowID>0))
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
            // if status is not zero, only update data
            // if status is zero, need to add shared[0] info also
//            if(nowState == 0)
//            {
//                if(sem_p(semid[0]))
//                {
//                    printf("sem_p fail.\n");
//                }

//                shared[0]->flag[0] = true;
//                endLoadTime = getTimeus();
//                shared[0]->dataDou[1] = endLoadTime;
//                shared[0]->dataInt[0] = numTrajs;
//                shared[0]->dataInt[1] = numPoints;
//                shared[0]->dataInt[2] = datasetNowID;

//                if(sem_v(semid[0]))
//                {
//                    printf("sem_v fail.\n");
//                }
//            }
            renewSystemState(numTrajs,numPoints,-1,-1);
            // output new system status
            cout << "Building index successful"<<endl;
            if(sem_p(semid[6]))
            {
                printf("sem_p fail.\n");
            }
            if(shared[6]->flag[1])
            {
                shared[6]->dataDou[1] = getTimeus();
                shared[6]->flag[1] = false;
                shared[6]->flag[0] = true;
            }
            if(sem_v(semid[6]))
            {
                printf("sem_p fail.\n");
            }
            indexIsExist = 1;
        }
        if(nowState == 0)
        {
            if(sem_p(semid[0]))
            {
                printf("sem_p fail.\n");
            }

            shared[0]->flag[0] = true;
            endLoadTime = getTimeus();
            shared[0]->dataDou[1] = endLoadTime;
            shared[0]->dataInt[0] = numTrajs;
            shared[0]->dataInt[1] = numPoints;
            shared[0]->dataInt[2] = datasetNowID;

            if(sem_v(semid[0]))
            {
                printf("sem_v fail.\n");
            }
            renewSystemState(numTrajs,numPoints,-1,-1);
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
        double batchStartTime=0,batchEndTime = 0;
        if(sem_p(semid[1]))
        {
            printf("sem_p fail.\n");
        }
        if((argc == 2 && argv[1][0]== 'f') || (shared[1]->flag[2]))
        {
            /*
            Load Schedular... If system is down, restart after data and index are all fine
            -----------------------------------------------------------------------------------
            */
            if(sem_v(semid[1]))
            {
                printf("sem_v fail.\n");
            }
            batchStartTime = getTimeus(); // batchStartTime should be record in Schedular

            if(systemMode==0)
            {
                sche = (Schedular*)malloc(sizeof(Schedular));
                sche->lastCompletedJob = -1;
                cout << "begin running schedular..." << endl;
                runSchedular(sche,g,tradb);
            }
            else
            {
                if((*stateData)==3)
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
            batchEndTime = getTimeus();
            if(sem_p(semid[1]))
            {
                printf("sem_p fail.\n");
            }
            shared[1]->dataDou[1] = getTimeus();
            shared[1]->flag[2]=false;
            shared[1]->flag[1]=true; // finish one
            shared[1]->flag[0]=true;
            if(sem_v(semid[1]))
            {
                printf("sem_v fail.\n");
            }
        }
        else
        {
            if(sem_v(semid[1]))
            {
                printf("sem_v fail.\n");
            }
        }


        if(sem_p(semid[7]))
        {
            printf("sem_p fail.\n");
        }
        if(shared[7]->flag[1])
        {
            shared[7]->flag[1]=0;
            shared[7]->flag[0] = 1;
            if(sem_v(semid[7]))
            {
                printf("sem_v fail.\n");
            }
            return -1;
        }
        else
        {
            if(sem_v(semid[7]))
            {
                printf("sem_v fail.\n");
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

    semid = semget(key+1, 1, IPC_CREAT|0666);
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
    // if trajN == -1 or pointN == -1, means not modified
    if(trajN>=0)
        numTrajs = trajN;
    if(pointN>=0)
        numPoints = pointN;
    if(runningType >=-1)
        nowState = runningType;
    if(queryIDRunning>=-1)
        nowQueryID = queryIDRunning;
    if(sem_p(semid[4]))
    {
        printf("sem_p fail.\n");
    }
    // if trajN == -1 or pointN == -1, means not modified
    if(trajN>=0)
        shared[4]->dataInt[0] = numTrajs;
    if(pointN>=0)
        shared[4]->dataInt[1] = numPoints;
    if(queryIDRunning >=-1)
        shared[4]->dataInt[2] = nowQueryID;
    if(runningType>=-1)
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

double getTimeus()
{
    struct timeval time;
    gettimeofday(&time,NULL);
    double ms =1000000*(time.tv_sec) + time.tv_usec;
    return ms;
}

// updateSM(SMDATA smdata, )
// It is meaningless to write a unified function to update SM because it does not simplify the code.
