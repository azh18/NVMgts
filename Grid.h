#pragma once
#include "Cell.h"
#include "ConstDefine.h"
#include "MBB.h"
#include "QueryResult.h"
#include "Trajectory.h"
#include <iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<vector>
#include "BufferManager.h"



class Grid{
public:
	MBB range;
	float cell_size; //length of a cell
	int cell_num_x,cell_num_y; //横竖各有多少个cell
	int cellnum; //upper(area(grid)/area(cell))，保证能放下所有cell
	Cell* cellPtr; //存储cell的入口
	ofstream fout;//文件输出接口
	int totalPointNum; //grid内点个数
	BufferManager buffer; //buffer，冷热区域
	Grid();
};

int initGrid(Grid *g,const MBB& mbb, float val_cell_size);
int addTrajectoryIntoCell(Grid *g, Trajectory &t);
int WhichCellPointIn(Grid *g, SamplePoint p);
int addDatasetToGrid(Grid *g,Trajectory* db,int traNum);
int writeCellsToFile(Grid *g,int* cellNo, int cellNum,string file);
//rangeQuery函数，输入Bounding box，输出轨迹编号和对应顺序下的采样点
int rangeQuery(Grid *g,MBB & bound, CPURangeQueryResult * ResultTable, int* resultSetSize);

