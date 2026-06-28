#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include  <iostream>
// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum=blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *buffer;

  //reading the entire disk block to the buffer
  int ret=loadBlockAndGetBufferPtr(&buffer);
  if(ret!=SUCCESS)
  {
	return ret;
  }

  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries,buffer + 16, 4);
  memcpy(&head->numAttrs,buffer + 20, 4);
  memcpy(&head->rblock,buffer + 12, 4);
  memcpy(&head->lblock,buffer + 8, 4);

  return SUCCESS;
}

//R=record block....callling the default blockbuffer constructor telling the constructor to return a blocknum by looking for a new block rather than just returning block no from exiting block
RecBuffer::RecBuffer() : BlockBuffer::BlockBuffer('R'){}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;
  // get the header using this.getHeader() function
  this->getHeader(&head);
  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  // read the block at this.blockNum into a buffer
  unsigned char *buffer;
  int ret=loadBlockAndGetBufferPtr(&buffer);
  if(ret!=SUCCESS)
  {
        return ret;
  }
  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer =buffer+HEADER_SIZE+slotCount+(recordSize*slotNum);

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec,int slotNum){
  struct HeadInfo head;
  this->getHeader(&head);
  int attrCount=head.numAttrs;
  int slotCount=head.numSlots;
  if(slotNum>=slotCount || slotNum<0)
  {
	return E_OUTOFBOUND;
  }
  unsigned char *buffer;
  int ret=loadBlockAndGetBufferPtr(&buffer);
  if(ret!=SUCCESS)
  {
	return ret;
  }
  int recordSize=attrCount*ATTR_SIZE;
  unsigned char* slotPointer=buffer+HEADER_SIZE+slotCount+(recordSize*slotNum);
  memcpy(slotPointer,rec,recordSize);
  StaticBuffer::setDirtyBit(this->blockNum);
  return SUCCESS;
}
/*
Used to load a block to the buffer and get a pointer to it.
NOTE: this function expects the caller to allocate memory for the argument
*/
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }
  //timeStamp update....
  //LRU Principple
  else
  {
	for(int i=0;i<BUFFER_CAPACITY;i++)
	{
		if(StaticBuffer::metainfo[i].free == false)
		{
			if(i==bufferNum)
			{
				//The time stamp of the recently accesed block is set to 0 indicating it should have less priority in case of an overwrite
				StaticBuffer::metainfo[i].timeStamp=0;
			}
			else
			{
				//the time stamp of the other blocks are incremented......
				(StaticBuffer::metainfo[i].timeStamp)++;
			}
		}
	}
  }

  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}
int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(ret!=SUCCESS)
    {
	return ret;
    }
    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
    bufferHeader->blockType=head->blockType;
    bufferHeader->pblock=head->pblock;
    bufferHeader->rblock=head->rblock;
    bufferHeader->lblock=head->lblock;
    bufferHeader->numEntries=head->numEntries;
    bufferHeader->numAttrs=head->numAttrs;
    bufferHeader->numSlots=head->numSlots;
    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    ret=StaticBuffer::setDirtyBit(this->blockNum);
    if(ret!=SUCCESS)
    {
	return ret;
    }
    return SUCCESS;
    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code

    // return SUCCESS;
}
int RecBuffer::getSlotMap(unsigned char *slotMap)
{
	unsigned char* bufferPtr;
	int ret=loadBlockAndGetBufferPtr(&bufferPtr);
	if(ret!=SUCCESS)
	{
		return ret;
	}
	struct HeadInfo head;
	this->getHeader(&head);
	int slotCount=head.numSlots;
	unsigned char* slotMapBufferPtr=bufferPtr+HEADER_SIZE;
	memcpy(slotMap,slotMapBufferPtr,slotCount);
	return SUCCESS;
}
int compareAttrs(union Attribute attr1,union Attribute attr2,int attrType)
{
	int diff=0;
	if(attrType==STRING)
	{
		diff=strcmp(attr1.sVal,attr2.sVal);
	}
	else
	{
		diff=attr1.nVal-attr2.nVal;
	}
	if(diff>0)
	{
		return 1;
	}
	else if(diff<0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}
int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(ret!=SUCCESS)
    {
	return ret;
    }
    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    // *((int32_t *)bufferPtr) = blockType;
    *((int32_t *)bufferPtr) = blockType;
    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
    StaticBuffer::blockAllocMap[this->blockNum]=blockType;
    // update dirty bit by calling StaticBuffer::setDirtyBit()
    ret=StaticBuffer::setDirtyBit(this->blockNum);
    // if setDirtyBit() failed
        // return the returned value from the call
    return ret;
    // return SUCCESS
}
int BlockBuffer::getFreeBlock(int blockType){

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int freeblock=-1;
    for(int i=0;i<DISK_BLOCKS;i++)
    {
	if(StaticBuffer::blockAllocMap[i] == UNUSED_BLK)
	{
		freeblock=i;
		break;
	}
    }
    // if no block is free, return E_DISKFULL.
    if(freeblock==-1)
    {
	return E_DISKFULL;
    }
    // set the object's blockNum to the block number of the free block.
    this->blockNum=freeblock;
    // find a free buffer using StaticBuffer::getFreeBuffer() .
    StaticBuffer::getFreeBuffer(this->blockNum);
    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHer() function.
    HeadInfo head;
    head.pblock=-1;
    head.lblock=-1;
    head.rblock=-1;
    head.numEntries=0;
    head.numAttrs=0;
    head.numSlots=0;

    int ret=setHeader(&head);
    if(ret!=SUCCESS)
    {
	return ret;
    }
    // update the block type of the block to the input block type using setBlockType().
    ret=setBlockType(blockType);
    if(ret!=SUCCESS)
    {
	return ret;
    }
    // return block number of the free block.
    return this->blockNum;
}
BlockBuffer::BlockBuffer(char blockType){
    // allocate a block on the disk and a buffer in memory to hold the new block of
    int ret=getFreeBlock(blockType);
    // given type using getFreeBlock function and get the return error codes if any.
    // set the blockNum field of the object to that of the allocated block
    this->blockNum=ret;
    // number if the method returned a valid block number,
    // otherwise set the error code returned as the block number.

    // (The caller must check if the constructor allocatted block successfully
    // by checking the value of block number field.)
}
int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using
       loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(ret!=SUCCESS)
    {
	return ret;
    }
    // get the header of the block using the getHeader() function
    HeadInfo head;
    ret=getHeader(&head);
    if(ret!=SUCCESS)
    {
	return ret;
    }
    int numSlots = head.numSlots;

    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`
    memcpy(bufferPtr+HEADER_SIZE,slotMap,numSlots);
    // update dirty bit using StaticBuffer::setDirtyBit
    // if setDirtyBit failed, return the value returned by the call
    ret=StaticBuffer::setDirtyBit(this->blockNum);
    return ret;
    // return SUCCESS
}
void BlockBuffer::releaseBlock(){

    // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
    if(this->blockNum == INVALID_BLOCKNUM)
    {
	return;
    }
    else
    {
        /* get the buffer number of the buffer assigned to the block
           using StaticBuffer::getBufferNum().
           (this function return E_BLOCKNOTINBUFFER if the block is not
           currently loaded in the buffer)
            */
	int bufferNum=StaticBuffer::getBufferNum(this->blockNum);
	if(bufferNum != E_BLOCKNOTINBUFFER)
	{
		StaticBuffer::metainfo[bufferNum].free = true;
	}
        // if the block is present in the buffer, free the buffer
        // by setting the free flag of its StaticBuffer::tableMetaInfo entry
        // to true.

        // free the block in disk by setting the data type of the entry
        // corresponding to the block number in StaticBuffer::blockAllocMap
        // to UNUSED_BLK.
	StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;
        // set the object's blockNum to INVALID_BLOCK (-1)
	this->blockNum = INVALID_BLOCKNUM;
     }

}
int BlockBuffer::getBlockNum()
{
	return this->blockNum;
}
