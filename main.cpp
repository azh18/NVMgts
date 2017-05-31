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
    p_init(800*1024*1024);

    if(argc == 2 && argv[1][0]== 'c'){

        if(p_get_bind_node(1,&sz)==NULL){
            printf("Cleaning Data...\n");
        }
        else{
            printf("Cleaning Data2...\n");
            int *stateData = (int*)p_get_bind_node(1,&sz);
            //p_free(stateData);
            *stateData = 1;
            printf("%d",(*stateData));
            tradb = (Trajectory*)p_get_bind_node(2,&sz);

            if(tradb != NULL)
                {
                    p_free(tradb);
                    printf("freed tradb");
                }
            sysInfo = (SysInfo*)p_get_bind_node(3,&sz);
            if(sysInfo!=NULL)
                p_free(sysInfo);
        }
        // p_clear();

        return 0;
    }


    //p_init(800*1024*1024-4);
	if((p_get_bind_node(1,&sz)==NULL)||(argc == 2 && argv[1][0]== 'r')){
        printf("Data not loaded, Loading Data...\n");
        int *stateData = (int*)p_malloc(sizeof(int));
        p_bind(1,stateData,sizeof(int));
        (*stateData) = 1; //build the stateData but not load in data
        printf("Allocating NVM...\n");
        tradb = (Trajectory*)p_malloc(sizeof(Trajectory)*MAX_TRAJ_SIZE);
        p_bind(2,tradb,sizeof(Trajectory));
        printf("Loading Data...\n");
        (*stateData) = 2; //NVM allocated
        PreProcess pp;
        pp.init("data_Small_SH.txt", "dataout.txt");
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
        int *stateData = (int*)p_get_bind_node(1,&sz);
        char *base = (char*)p_get_base();
        //check if the process is finished
        if((*stateData)==1){
            // malloc and load data again
            printf("Data not loaded, Loading Data2...\n");
            tradb = (Trajectory*)p_malloc(sizeof(Trajectory)*MAX_TRAJ_SIZE);
            p_bind(2,tradb,sizeof(Trajectory));
            (*stateData) = 2; //NVM allocated
            PreProcess pp;
            pp.init("data_Small_SH.txt", "dataout.txt");
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
            pp.init("data_Small_SH.txt", "dataout.txt");
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
            // load finished, only need bind
            printf("Data loaded, Recovering Data4...\n");
            tradb = (Trajectory*)p_get_bind_node(2,&sz);
            sysInfo = (SysInfo*)p_get_bind_node(3,&sz);
            printf("location:%f,%f;time:%d;Tid:%d\n",tradb[3].points[2].lat,tradb[3].points[2].lon,tradb[3].points[2].time,tradb[3].points[2].tid);
        }

	}


	cout << WriteTrajectoryToFile("dataOut.txt", sysInfo->maxTid) << endl;
	cout << "read trajectory success!" << endl << "Start building cell index" << endl;
	Grid* g = new Grid(MBB(sysInfo->xmin, sysInfo->ymin, sysInfo->xmax, sysInfo->ymax), 0.003);
	g->addDatasetToGrid(tradb, sysInfo->maxTid);
	int count = 0;
	for (int i = 0; i <= g->cellnum - 1; i++) {
		if (g->cellPtr[i].subTraNum == 0)
			count++;
	}
	//cout << "zero num:" << count << "total" << g->cellnum << endl;
	//int temp[7] = { 553,554,555,556,557,558,559 };
	//int sizetemp = 7;
	//g->writeCellsToFile(temp, sizetemp, "111.txt");
    Schedular schedular;
    schedular.run(g,tradb);
	CPURangeQueryResult* resultTable=NULL;
	int RangeQueryResultSize = 0;
	MBB queryMbb = MBB(121.4, 31.15, 121.45, 31.20);
	g->rangeQuery(queryMbb, resultTable, &RangeQueryResultSize);


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
