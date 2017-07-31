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
#include "ConstDefine.h"

using namespace std;

map<string, tidLinkTable*> vidTotid;
map<string, tidLinkTable*>::iterator iter;

typedef struct SysInfo{
    double ymin;
    double ymax;
    double xmin;
    double xmax;
    int maxTid;
}SysInfo;

//global
Trajectory* tradb=NULL;
int tradbNVMID = -1;
string baseDate = "2014-07-01";
SysInfo *sysInfo=NULL;
int *stateData = NULL;

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

int main(int argc, char **argv)
{
    //printf("hellp");


	int WriteTrajectoryToFile(string outFileName, int numTra);
	cout << "Hello world!" << endl;
	cout << sizeof(Cell) << endl;
	//ifstream fin;
	//float lon1, lat1, lon2, lat2;
	//lat1 = +34.15265;
	//lon1 = +113.10221;
	//lat2 = +35.15221;
	//lon2 = +113.10222;
	//cout << calculateDistance(lat1, lon1, lat2, lon2) << endl;
	int sz;
    p_init(30*1024*1024);

    //clean the data but failed
    if(argc == 2 && argv[1][0]== 'c'){
        //*stateData = 3;
        if(p_get(1,sizeof(int))==NULL){
            printf("Cleaning Data0...\n");
        }
        else{
            printf("Cleaning Data2...\n");
            stateData = (int*)p_get(1,sizeof(int));
            //p_free(stateData);

            printf("%d",(*stateData));
            tradb = (Trajectory*)p_get_bind_node(2,&sz);

            if(*stateData > 1)
                {
                    p_free(tradb);
                    *stateData = 1;
                    printf("freed tradb");
                }

            sysInfo = (SysInfo*)p_get_bind_node(3,&sz);
            if(sysInfo != NULL)
                p_free(sysInfo);
            *stateData = 1;

        }
        cout << "finishing deleting..." << endl;
        //(*stateData) = 1;
        return 0;
    }


    /*
    Load Data...
    If system is down, recover the pointer from NVM
    -----------------------------------------------------------------------------------
    */
	if((p_get(1,sizeof(int))==NULL)||(argc == 2 && argv[1][0]== 'r')){
        printf("Data not loaded, Loading Data...\n");

//        stateData = (int*)p_malloc(sizeof(int));
//        p_bind(1,stateData,sizeof(int));
        stateData = (int*)p_new(1,sizeof(int));
        //p_bind(1,stateData,sizeof(int));
        (*stateData) = 1; //build the stateData but not load in data
        printf("Allocating NVM...\n");
        tradb = (Trajectory*)p_malloc(sizeof(Trajectory)*MAX_TRAJ_SIZE);
	printf("Allocate:%p,size:%d\n",tradb,MAX_TRAJ_SIZE*sizeof(Trajectory));
        p_bind(2,tradb,sizeof(Trajectory));
        printf("Loading Data...\n");
        (*stateData) = 2; //NVM allocated
        PreProcess pp;
        pp.init("data_SSmall_SH.txt", "dataout.txt");
        sysInfo = (SysInfo*)p_malloc(sizeof(SysInfo));
        p_bind(3,sysInfo,sizeof(SysInfo));
        sysInfo->xmin = pp.xmin;
        sysInfo->xmax = pp.xmax;
        sysInfo->ymin = pp.ymin;
        sysInfo->ymax = pp.ymax;
        sysInfo->maxTid = pp.maxTid;
        (*stateData) = 3; //build data finished
        printf("Load Data finished\n");
	}
	else{
        stateData = (int*)p_get(1,sizeof(int));
        char *base = (char*)p_get_base();
        //check if the process is finished
        if((*stateData)==1){
            // malloc and load data again
            printf("Data not loaded, Loading Data2...\n");
            tradb = (Trajectory*)p_malloc(sizeof(Trajectory)*MAX_TRAJ_SIZE);
	    printf("Allocate:%p",tradb);
            printf("malloc: %p\n", tradb);
            if(tradb == NULL){
                printf("error allocate traDB\n");
            }
            p_bind(2,tradb,sizeof(Trajectory));
            (*stateData) = 2; //NVM allocated
            PreProcess pp;
            pp.init("data_SSmall_SH.txt", "dataout.txt");
            sysInfo = (SysInfo*)p_malloc(sizeof(SysInfo));
            p_bind(3,sysInfo,sizeof(SysInfo));
            sysInfo->xmin = pp.xmin;
            sysInfo->xmax = pp.xmax;
            sysInfo->ymin = pp.ymin;
            sysInfo->ymax = pp.ymax;
            sysInfo->maxTid = pp.maxTid;
            (*stateData) = 3; //build data finished
            printf("Load Data finished\n");
        }
        else if((*stateData)==2){
            // load data again
            printf("Data not loaded, Loading Data3...\n");
            tradb = (Trajectory*)p_get_bind_node(2,&sz);
            PreProcess pp;
            pp.init("data_SSmall_SH.txt", "dataout.txt");
            sysInfo = (SysInfo*)p_malloc(sizeof(SysInfo));
            p_bind(3,sysInfo,sizeof(SysInfo));
            sysInfo->xmin = pp.xmin;
            sysInfo->xmax = pp.xmax;
            sysInfo->ymin = pp.ymin;
            sysInfo->ymax = pp.ymax;
            sysInfo->maxTid = pp.maxTid;
            (*stateData) = 3;
            printf("Load Data finished\n");
        }
        else{
            // stateData>=3, load finished, only need bind and query
            printf("Data loaded, Recovering Data4...\n");
            tradb = (Trajectory*)p_get_bind_node(2,&sz);
            sysInfo = (SysInfo*)p_get_bind_node(3,&sz);
            printf("location:%f,%f;time:%d;Tid:%d\n",tradb[3].points[2].lat,tradb[3].points[2].lon,tradb[3].points[2].time,tradb[3].points[2].tid);
        }

	}

    /*
    Build Grid Index...
    -----------------------------------------------------------------------------------
    */

	cout << WriteTrajectoryToFile("dataOut.txt", sysInfo->maxTid) << endl;
	cout << "read trajectory success!" << endl << "Start building cell index" << endl;
	//Grid* g = new Grid(MBB(sysInfo->xmin, sysInfo->ymin, sysInfo->xmax, sysInfo->ymax), 0.003);
	Grid *g = (Grid*)malloc(sizeof(Grid));
	initGrid(g,MBB(sysInfo->xmin, sysInfo->ymin, sysInfo->xmax, sysInfo->ymax), 0.003);
	//g->addDatasetToGrid(tradb, sysInfo->maxTid);
	addDatasetToGrid(g,tradb, sysInfo->maxTid);
	int count = 0;
	for (int i = 0; i <= g->cellnum - 1; i++) {
		if (g->cellPtr[i].subTraNum == 0)
			count++;
	}
	//cout << "zero num:" << count << "total" << g->cellnum << endl;
	//int temp[7] = { 553,554,555,556,557,558,559 };
	//int sizetemp = 7;
	//g->writeCellsToFile(temp, sizetemp, "111.txt");
    cout << "Building index successful"<<endl;
    /*
    Load Schedular... If system is down, restart after data and index are all fine
    -----------------------------------------------------------------------------------
    */
    Schedular *sche = NULL;
    if(*stateData==3){
        //stateData==3 means we have not build a schedular, haven't run
        sche = (Schedular*)p_malloc(sizeof(Schedular));
        p_bind(4,sche,sizeof(Schedular));
        *stateData = 4;
        sche->lastCompletedJob = -1;
        cout << "begin running schedular..." << endl;
        runSchedular(sche,g,tradb);
    }
    else{
        //stateData>=4 means schedular has been in NVM
        sche = (Schedular*)p_get_bind_node(4,&sz);
        cout << "begin continuing schedular..." << endl;
        runSchedular(sche,g,tradb);
    }
//    Schedular schedular;
//    runSchedular(&schedular,g,tradb);
    //schedular.run(g,tradb);

	CPURangeQueryResult* resultTable=NULL;
	int RangeQueryResultSize = 0;
	MBB queryMbb = MBB(121.4, 31.15, 121.45, 31.20);
	//g->rangeQuery(queryMbb, resultTable, &RangeQueryResultSize);
	rangeQuery(g,queryMbb, resultTable, &RangeQueryResultSize);


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

int WriteTrajectoryToFile(string outFileName,int numTra) {
	ofstream fout;
	fout.open(outFileName, ios_base::out);
	for (int i = 1; i <= numTra; i++) {
		fout << i << ": ";
		for (int j = 0; j <= tradb[i].length - 1; j++) {
			fout << tradb[i].points[j].lon << "," << tradb[i].points[j].lat << ";";
		}
		fout << endl;
	}
	fout.close();
	return 1;
}
