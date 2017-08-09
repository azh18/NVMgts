#include "Grid.h"


extern Trajectory* tradb;


int initGrid(Grid *g,const MBB& mbb,float val_cell_size)
{
	g->range = mbb;
	g->cell_size = val_cell_size;
	//横向有多少个cell
	g->cell_num_x = (int)((mbb.xmax - mbb.xmin) / val_cell_size) + 1;
	//纵向有多少个cell
	g->cell_num_y = (int)((mbb.ymax - mbb.ymin) / val_cell_size) + 1;
	g->cellnum = g->cell_num_x*g->cell_num_y;
	//g->cellPtr = new Cell[cellnum];
	g->cellPtr = (Cell*)malloc(sizeof(Cell)*g->cellnum);
	//注意cell编号是从(xmin,ymax)开始的，而不是(xmin,ymin)
	for (int i = 0; i <= g->cell_num_y - 1; i++)
	{
		for (int j = 0; j <= g->cell_num_x - 1; j++)
		{
			int cell_idx = i*g->cell_num_x + j;
			initialCell(&(g->cellPtr[cell_idx]),i, j, MBB(g->range.xmin + g->cell_size*j, g->range.ymax - g->cell_size*(i+1), g->range.xmin + g->cell_size*(j + 1), g->range.ymax - g->cell_size*(i)));
			//g->cellPtr[cell_idx].initial(i, j, MBB(g->range.xmin + g->cell_size*j, g->range.ymax - g->cell_size*(i+1), g->range.xmin + g->cell_size*(j + 1), g->range.ymax - g->cell_size*(i)));
		}
	}
	return 0;
}

//把轨迹t打碎成子轨迹，添加到cell里面
int addTrajectoryIntoCell(Grid *g, Trajectory &t)
{
	if (t.length == 0)
		return 1;//空轨迹
	SamplePoint p = t.points[0];
	int lastCellNo = WhichCellPointIn(g,p);
	int lastCellStartIdx = 0;
	int nowCellNo;
	for (int i = 1; i <= t.length - 1; i++)
	{
		p = t.points[i];
		nowCellNo = WhichCellPointIn(g,p);
		if (i == t.length - 1)
		{
			if (lastCellNo == nowCellNo)
			{
				addSubTra(&(g->cellPtr[nowCellNo]),t.tid, lastCellStartIdx, i, i - lastCellStartIdx + 1);
				//g->cellPtr[nowCellNo].addSubTra(t.tid, lastCellStartIdx, i, i - lastCellStartIdx + 1);
				return 0;
			}
			else
			{
				addSubTra(&(g->cellPtr[lastCellNo]),t.tid, lastCellStartIdx, i - 1, i - 1 - lastCellStartIdx + 1);
				addSubTra(&(g->cellPtr[nowCellNo]),t.tid, i, i, 1);
//				cellPtr[lastCellNo].addSubTra(t.tid, lastCellStartIdx, i - 1, i - 1 - lastCellStartIdx + 1);
//				cellPtr[nowCellNo].addSubTra(t.tid, i, i, 1);
				return 0;
			}
		}
		else
		{
			if (lastCellNo == nowCellNo)
				continue;
			else
			{
				addSubTra(&(g->cellPtr[lastCellNo]),t.tid, lastCellStartIdx, i - 1, i - 1 - lastCellStartIdx + 1);
				//cellPtr[lastCellNo].addSubTra(t.tid, lastCellStartIdx, i - 1, i - 1 - lastCellStartIdx + 1);
				lastCellNo = nowCellNo;
				lastCellStartIdx = i;
			}
		}
	}
	return 0;
}

int WhichCellPointIn(Grid *g, SamplePoint p)
{
	//注意cell编号是从(xmin,ymax)开始的，而不是(xmin,ymin)
	int row = (int)((g->range.ymax - p.lat) / g->cell_size);
	int col = (int)((p.lon - g->range.xmin) / g->cell_size);
	return row*g->cell_num_x + col;
}

int addDatasetToGrid(Grid *g,Trajectory* db,int traNum)
{
	//注意，轨迹编号从1开始
	int pointCount = 0;
	for (int i = 1; i <= traNum; i++)
	{
		addTrajectoryIntoCell(g, db[i]);
	}
	for (int i = 0; i <= g->cellnum - 1; i++)
	{
		//cellPtr[i].buildSubTraTable();
		buildSubTraTable(&(g->cellPtr[i]));
		pointCount += g->cellPtr[i].totalPointNum;
	}
	g->totalPointNum = pointCount;


	return 0;
}

int writeCellsToFile(Grid *g,int* cellNo, int cellNum,string file)
// under editing....
{
	g->fout.open(file, ios_base::out);
	for (int i = 0; i <= cellNum - 1; i++)
	{
		int outCellIdx = cellNo[i];
		cout << outCellIdx << ": " << "[" << g->cellPtr[outCellIdx].mbb.xmin << "," <<g->cellPtr[outCellIdx].mbb.xmax << "," << g->cellPtr[outCellIdx].mbb.ymin << "," << g->cellPtr[outCellIdx].mbb.ymax << "]" << endl;
		for (int j = 0; j <= g->cellPtr[outCellIdx].subTraNum - 1; j++)
		{
			int tid = g->cellPtr[outCellIdx].subTraTable[j].traID;
			int startpid = g->cellPtr[outCellIdx].subTraTable[j].startpID;
			int endpid = g->cellPtr[outCellIdx].subTraTable[j].endpID;
			for (int k = startpid; k <= endpid; k++)
			{
				cout << tradb[tid].points[k].lat << "," << tradb[tid].points[k].lon << ";";
			}
			cout << endl;
		}
	}
	return 0;
}

//int Grid::rangeQuery(MBB & bound, int * ResultTraID, SamplePoint ** ResultTable,int* resultSetSize,int* resultTraLength)
int rangeQuery(Grid *g,MBB & bound, CPURangeQueryResult * ResultTable, int* resultSetSize)
{
	sleep(1);
	//这部分要移植到gpu上，尽量用底层函数写
	//为了可比较，在这个函数内仅仅要求把轨迹查出来就行了，result的组织交由QueryResult类来做
	//判断range是否超出地图
	ResultTable = (CPURangeQueryResult*)malloc(sizeof(CPURangeQueryResult));
	ResultTable->traid = -1; //table开头traid为-1 flag
	ResultTable->next = NULL;
	CPURangeQueryResult* newResult,* nowResult;
	nowResult = ResultTable;
	if (g->range.intersect(bound) != 2)
		return 1;
	else
	{
		int g1, g2, g3, g4; //box的顶点所在行列
		int a, b, c, d;//box的顶点所在格子号
		int *candidatesCellID=NULL,*resultsCellID=NULL,*directResultsCellID=NULL;//候选格子，默认为空
		int m, n;//m为grid列数，n为行数
		int candidateSize = 0;//candidate个数
		int DirectresultSize = 0;//结果个数
		int counter = 0;//计数器
		m = g->cell_num_x;
		n = g->cell_num_y;
		g1 = (int)((bound.xmin - g->range.xmin) / g->cell_size);
		g2 = (int)((bound.xmax - g->range.xmin) / g->cell_size);
		g3 = (int)((g->range.ymax - bound.ymax) / g->cell_size);
		g4 = (int)((g->range.ymax - bound.ymin) / g->cell_size);
		//for test
		//g1 = test[0];
		//g2 = test[1];
		//g3 = test[2];
		//g4 = test[3];
		//m = 10;
		//n = 10;

		a = g1 + g3*m;
		b = g2 + g3*m;
		c = g1 + g4*m;
		d = g2 + g4*m;

		if (a == b)
		{
			candidateSize = (c - a) / m + 1;
		}
		else
		{
			if (a == c)
				candidateSize = (b - a) + 1;
			else
				candidateSize = ((c - a) / m + 1) * 2 + (b - a + 1) * 2 - 4;
		}
		//把bounding box的线经过的cell加入candidates
		candidatesCellID = (int*)malloc(sizeof(int)*candidateSize);
		counter = 0;
		for (int i = a; i <= b; i++)
		{
			candidatesCellID[counter] = i;
			counter++;
		}
		for (int i = c; i <= d; i++)
		{
			candidatesCellID[counter] = i;
			counter++;
		}
		if (g4 - g3 >= 2)
		{
			for (int i = a + m; i <= a + (g4 - g3- 1)*m; i = i + m)
			{
				candidatesCellID[counter] = i;
				counter++;
			}
			for (int i = b + m; i <= b + (g4 - g3- 1)*m; i = i + m)
			{
				candidatesCellID[counter] = i;
				counter++;
			}
		}
		if (counter != candidateSize)
			cerr << "size error in range query candidates cell" << endl;

		//一些直接是result
		DirectresultSize = (b - a - 1)*(g4 - g3 - 1);
		counter = 0;
		directResultsCellID = (int*)malloc(DirectresultSize * sizeof(int));
		if (b >= a + 2 && c >= a + 2 * m)
		{
			for (int i = a + 1; i <= b - 1; i++)
			{
				for (int j = 1; j <= g4 - g3 - 1; j++)
				{
					directResultsCellID[counter] = i + j*m;
					counter++;
				}
			}
		}
		if (counter != DirectresultSize)
			cerr << "size error in range query directresult cell" <<counter<<","<<candidateSize<< endl;

		//对所有candidateCell检测，可并行
		counter = 0;
		for (int i = 0; i <= candidateSize - 1; i++)
		{
			Cell &ce = g->cellPtr[candidatesCellID[i]];
			for (int j = 0; j <= ce.subTraNum - 1; j++)
			{
				int traid = ce.subTraTable[j].traID;
				int startIdx = ce.subTraTable[j].startpID;
				int endIdx = ce.subTraTable[j].endpID;
				for (int k = startIdx; k <= endIdx; k++)
				{
					if (bound.pInBox(tradb[traid].points[k].lon, tradb[traid].points[k].lat))//该点在bound内
					{
						newResult = (CPURangeQueryResult*)malloc(sizeof(CPURangeQueryResult));
						if (newResult == NULL)
							return 2; //分配内存失败
						newResult->traid = tradb[traid].points[k].tid;
						newResult->x = tradb[traid].points[k].lon;
						newResult->y = tradb[traid].points[k].lat;
						newResult->next = NULL;
						nowResult->next = newResult;
						nowResult = newResult;
						counter++;
					}
				}
			}
		}

		//直接作为result的cell加进resulttable
		for (int i = 0; i <= DirectresultSize - 1; i++)
		{
			Cell &ce = g->cellPtr[directResultsCellID[i]];
			for (int j = 0; j <= ce.subTraNum - 1; j++)
			{
				int traid = ce.subTraTable[j].traID;
				int startIdx = ce.subTraTable[j].startpID;
				int endIdx = ce.subTraTable[j].endpID;
				for (int k = startIdx; k <= endIdx; k++)
				{
					newResult = (CPURangeQueryResult*)malloc(sizeof(CPURangeQueryResult));
					newResult->traid = tradb[traid].points[k].tid;
					newResult->x = tradb[traid].points[k].lon;
					newResult->y = tradb[traid].points[k].lat;
					newResult->next = NULL;
					nowResult->next = newResult;
					nowResult = newResult;
					counter++;
				}
			}
		}
		(*resultSetSize) = counter;
		//输出结果
		//CPURangeQueryResult* pNow = ResultTable;
//		while (pNow->next != NULL) {
//			printf("%f,%f,%d\n", pNow->next->x, pNow->next->y, pNow->next->traid);
//			pNow = pNow->next;
//		}
	}
	return 0;
}


