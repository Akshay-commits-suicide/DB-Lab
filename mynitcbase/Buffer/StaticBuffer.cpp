#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];
StaticBuffer::StaticBuffer() {

  for(int i=0;i<4;i++)
  {
	Disk::readBlock(blockAllocMap+i*BLOCK_SIZE,i);
  }
  //initialise all metadata of buffer blocks
  for (int i=0;i<BUFFER_CAPACITY;i++) {
    metainfo[i].free = true;
    metainfo[i].dirty=false;
    metainfo[i].timeStamp=-1;
    metainfo[i].blockNum=-1;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {
   //Writing the modified block allocation map back into disk
    for(int i=0;i<4;i++)
    {
	Disk::writeBlock(blockAllocMap+i*BLOCK_SIZE,i);
    }
   //Writing back all the dirty blocks back into disk
   for(int i=0;i<BUFFER_CAPACITY;i++)
   {
	if(metainfo[i].free == false && metainfo[i].dirty == true)
	{
		Disk::writeBlock(blocks[i],metainfo[i].blockNum);
	}
   }
}
int StaticBuffer::setDirtyBit(int blockNum){
   //Block numbers are marked true on their dirty bits if they have to be written back onto the disk at the disk deconstructor phase...
   int bufferNum=StaticBuffer::getBufferNum(blockNum);
   if(bufferNum == E_BLOCKNOTINBUFFER || bufferNum == E_OUTOFBOUND)
   {
	return bufferNum;
   }
   else
   {
	metainfo[bufferNum].dirty=true;
	return SUCCESS;
   }
}
int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum > DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }
  // iterate through all the blocks in the StaticBuffer
  // find the first free block in the buffer (check metainfo)
  // assign allocatedBuffer = index of the free block
  for(int i=0;i<BUFFER_CAPACITY;i++)
  {
	if(metainfo[i].free == false)
	{
		(metainfo[i].timeStamp)++;
	}
  }
  int allocatedBuffer=0;
  while(allocatedBuffer<BUFFER_CAPACITY && !(metainfo[allocatedBuffer].free))
  {
	allocatedBuffer++;
  }
  if(allocatedBuffer==BUFFER_CAPACITY)
  {
	int max_ind=0;
	for(int i=0;i<BUFFER_CAPACITY;i++)
	{
		if(metainfo[i].free == false && metainfo[i].timeStamp > metainfo[max_ind].timeStamp)
		{
			max_ind=i;
		}
	}
	if(metainfo[max_ind].dirty == true)
	{
		Disk::writeBlock(blocks[max_ind],metainfo[max_ind].blockNum);
	}
	allocatedBuffer=max_ind;
  }
  	metainfo[allocatedBuffer].free = false;
  	metainfo[allocatedBuffer].blockNum = blockNum;
	metainfo[allocatedBuffer].dirty = false;
	metainfo[allocatedBuffer].timeStamp = 0;
  	return allocatedBuffer;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
  if (blockNum < 0 || blockNum > DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  int bufferIndex=0;
  while(bufferIndex<BUFFER_CAPACITY && metainfo[bufferIndex].blockNum!=blockNum)
  {
	bufferIndex++;
  }
  if(bufferIndex>=BUFFER_CAPACITY)
  {
  	return E_BLOCKNOTINBUFFER;
  }
  else
  {
	return bufferIndex;
  }
}
