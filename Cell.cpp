#include "Cell.h"



//Cell::Cell(int x, int y, const MBB& val_mbb) {
//	cell_x = x;
//	cell_y = y;
//	mbb = val_mbb;
//	subTraNum = 0;
//	subTraPtr = NULL;
//}

bool initialCell(Cell *c,int x, int y, const MBB& val_mbb)
{
	c->cell_x = x;
	c->cell_y = y;
	c->mbb = val_mbb;
	c->subTraNum = 0;
	c->totalPointNum = 0;
	c->subTraEntry.next = NULL;
	c->subTraPtr = &(c->subTraEntry);
	c->subTraTable = NULL;
	return true;
}

int addSubTra(Cell *c,int traID, int startIdx, int endIdx, int numOfPoints)
{
	subTra* newSubTra = (subTra*)malloc(sizeof(subTra));//记得free！！！
	if (newSubTra == NULL)
		return 1;
	newSubTra->numOfPoint = numOfPoints;
	newSubTra->traID = traID;
	newSubTra->startpID = startIdx;
	newSubTra->endpID = endIdx;
	newSubTra->next = NULL;
	c->subTraPtr->next = newSubTra;
	c->subTraPtr = newSubTra;
	c->subTraNum++;
	return 0;
}

int buildSubTraTable(Cell *c)//读取完所有轨迹之后用数组存储
{
	int countPoints = 0;
	c->subTraTable = (subTra*)malloc(sizeof(subTra)*c->subTraNum);
	if (c->subTraTable == NULL)
		return 1;
	subTra* ptr = c->subTraEntry.next;
	subTra* nextptr;
	int idx = 0;
	while (ptr!= NULL) {
		nextptr = ptr->next;
		c->subTraTable[idx] = (*ptr);
		countPoints += (*ptr).numOfPoint;
		free(ptr);
		ptr = nextptr;
		idx++;
	}

	if (idx != c->subTraNum) //debug info
	{
		cout << idx << "vs" << endl;
		getchar();
	}
		// throw("error in cell buildSubTraTable");
	c->totalPointNum = countPoints;
	return 0;
}

int writeCellToFile(Cell *c,string fileName)
{
	return 0;
}


