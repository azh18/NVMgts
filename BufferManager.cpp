#include "BufferManager.h"

BufferManager::BufferManager()
{
	//ctor
}

subTraBuffer::subTraBuffer(){
}

cellBlockBuffer::cellBlockBuffer()
{
}


void BufferManager::remove(CacheNode *node)
{
	CacheNode *now = node;
	if(node->prev != NULL)
	{
		node->prev->next = node->next;
	}
	else
	{
		head = node->next;
	}

	if(node->next !=NULL)
	{
		node->next->prev = node->prev;
	}
	else
	{
		tail = node->prev;
	}
}

void BufferManager::insertToHead(CacheNode *node){
        node->next = head;
        node->prev = NULL;
        if(head != NULL){
		head->prev = node;
        }
        head = node;
        if(tail == NULL)
		tail = head;
}

int BufferManager::getKey(int key, Cell *ce, Trajectory* db){
        this->bufferStateIterTool = this->bufferState.find(key);
        if(this->bufferStateIterTool != this->bufferState.end())
        {
		CacheNode* node = this->bufferStateIterTool->second;
		this->remove(node);
		this->insertToHead(node);
		return 1; //在DRAM内
        }
        else if (this->cellFetchTime[key] >= this->thresReadTime)
        {
                if(this->bufferState.size()>=this->maxCellInDRAM)
                {
			//删除链表末尾的元素
			this->bufferStateIterTool = this->bufferState.find(this->tail->key);
                        this->bufferState.erase(this->bufferStateIterTool);
			this->bufferIterTool = this->bufferData.find(this->tail->key);
			this->bufferData.erase(this->bufferIterTool);
			this->cellFetchTime[this->tail->key] = 0;
			CacheNode *oldTail = this->tail;
			this->remove(this->tail);
			free(oldTail);
                }
                //增加新元素
                CacheNode* node = (CacheNode*)malloc(sizeof(CacheNode));
                node->key = key;
                this->insertToHead(node);
                this->bufferState[key] = node;
                cellBlockBuffer t,*temp;
                this->bufferData[key] = t;
                temp = &bufferData[key];
                temp->cellID = key;
                temp->subTraNum = ce->subTraNum;
		for(int subT =0; subT<=temp->subTraNum-1;subT++)
		{
                        subTraBuffer sub;
                        sub.length = ce->subTraTable[subT].numOfPoint;
                        int traID = ce->subTraTable[subT].traID;
                        sub.subTraID = traID;
                        temp->subTraData.push_back(sub);
			for(int idx = ce->subTraTable[subT].startpID;idx<=ce->subTraTable[subT].endpID;idx++)
			{
                                temp->subTraData[temp->subTraData.size()-1].points.push_back(db[traID].points[idx]);
			}
		}
		return 1; //在DRAM内
        }
        else
        {
		this->cellFetchTime[key]++;
		return 0; //意为不在DRAM内
        }

}

