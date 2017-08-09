#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H
#include <vector>
#include <map>
#include "ConstDefine.h"
#include "SamplePoint.h"
#include "Trajectory.h"
#include "Cell.h"


class subTraBuffer
{
public:
	int subTraID;
	int length;
	std::vector<SamplePoint> points;
	subTraBuffer();
};

class cellBlockBuffer
{
public:
	int cellID;
	int subTraNum;
	std::vector<subTraBuffer> subTraData;
	cellBlockBuffer();
};

typedef struct CacheNode
{
	int key;
	CacheNode *prev,*next;
} CacheNode;


class BufferManager  //利用LRU保存最近的频繁读取的cell
{
public:
	int maxCellInDRAM;
	int thresReadTime;
	std::map<int,cellBlockBuffer> bufferData; //DRAM中保存的轨迹的buffer
	std::map<int,cellBlockBuffer>::iterator bufferIterTool;

	CacheNode* head,*tail;
	std::map<int,CacheNode*> bufferState;
	std::map<int,CacheNode*>::iterator bufferStateIterTool;

	std::vector<int> cellFetchTime;


	BufferManager();
	void remove(CacheNode *node); //
	void insertToHead(CacheNode *node);
        int getKey(int key, Cell *ce, Trajectory* db);

};


#endif // BUFFERMANAGER_H
