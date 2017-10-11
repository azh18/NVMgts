#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H
#include <vector>
#include <unordered_map>
#include "ConstDefine.h"
#include "SamplePoint.h"
#include "Trajectory.h"
#include "Cell.h"


class subTraBuffer
{
public:
	int subTraID;
	int length;
	std::vector<SamplePoint> points; // vector比静态数组慢，所以改为静态数组
	//SamplePoint points[MAXLENGTH];
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
	std::unordered_map<int,cellBlockBuffer> bufferData; //DRAM中保存的轨迹的buffer
	std::unordered_map<int,cellBlockBuffer>::iterator bufferIterTool;

	CacheNode* head,*tail;
	std::unordered_map<int,CacheNode*> bufferState;
	std::unordered_map<int,CacheNode*>::iterator bufferStateIterTool;

	std::vector<int> cellFetchTime;


	BufferManager();
	void remove(CacheNode *node); //
	void insertToHead(CacheNode *node);
        int getKey(int key, Cell *ce, Trajectory* db);

};


#endif // BUFFERMANAGER_H
