#include "BlockAccess.h"

#include <cstring>

#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
	RecId prevRecId;
	RelCacheTable::getSearchIndex(relId,&prevRecId);
	int block,slot;
    // let block and slot denote the record id of the record being currently checked
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)
	RelCatEntry relCatEntry;
	RelCacheTable::getRelCatEntry(relId,&relCatEntry);
	block=relCatEntry.firstBlk;
	slot=0;
        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)

        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)
	block=prevRecId.block;
	slot=prevRecId.slot+1;
        // block = search index's block
        // slot = search index's slot + 1
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
	//getting the header
	RecBuffer iniblock(block);
	HeadInfo header;
	iniblock.getHeader(&header);
	//std::cout<<header.numSlots<<" "<<slot<<" "<<block<<" "<<header."\n";
	//Proceed to the adjacent block if the slotsize is lessthan slotnum
	if(slot>=header.numSlots)
	{
		block=header.rblock;
		slot=0;
		continue;
	}
	//get slotmap
	unsigned char slotMap[header.numSlots];
	iniblock.getSlotMap(slotMap);
	//traverse through slots if slot is unoccupied
	if(slotMap[slot]==SLOT_UNOCCUPIED)
	{
		slot++;
		continue;
	}
	//Retriving the record with the given blocknum and slotnum
	Attribute record[header.numAttrs];
	iniblock.getRecord(record,slot);
	//to retrive the attribute offset of the attribute with the given name from a record
	AttrCatEntry attrCatEntry;
	AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
	int attrOffset=attrCatEntry.offset;
	//using the attribute offset,we are comparing the values of actual record attribute and the parsed attribute value to check for equality
	int cmpVal=compareAttrs(record[attrOffset],attrVal,attrCatEntry.attrType);
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
	    RecId recId;
	    recId.block=block;
	    recId.slot=slot;
	    RelCacheTable::setSearchIndex(relId,&recId);
            return recId;
        }

        slot++;
    }
    // no record in the relation with Id relid satisfies the given condition
    RecId recId;
    recId.block=-1;
    recId.slot=-1;
    RelCacheTable::resetSearchIndex(relId);
    return recId;
}
int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal,newName);
    // search the relation catalog for an entry with "RelName" = newRelationName
    RecId recId=BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,newRelationName,EQ);
    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    if(recId.block!=-1 && recId.slot!=-1)
    {
 	 return E_RELEXIST;
    }
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal,oldName);
    // search the relation catalog for an entry with "RelName" = oldRelationName
    recId=BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,oldRelationName,EQ);
    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    if(recId.block==-1 && recId.slot==-1)
    {
	return E_RELNOTEXIST;
    }
    RecBuffer relCatBuffer(recId.block);
    Attribute relRecord[RELCAT_NO_ATTRS];
    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    relCatBuffer.getRecord(relRecord,recId.slot);
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    strcpy(relRecord[RELCAT_REL_NAME_INDEX].sVal,newName);
    // set back the record value using RecBuffer.setRecord
    relCatBuffer.setRecord(relRecord,recId.slot);
    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numAttrs=relRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    for(int i=0;i<numAttrs;i++)
    {
    //    linearSearch on the attribute catalog for relName = oldRelationName
	  recId=BlockAccess::linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,oldRelationName,EQ);
    //    get the record using RecBuffer.getRecord
	  if(recId.block==-1 && recId.slot==-1)
	  {
		break;
	  }
	  RecBuffer attrCatBuffer(recId.block);
	  Attribute attrRecord[ATTRCAT_NO_ATTRS];
	  attrCatBuffer.getRecord(attrRecord,recId.slot);
    //    update the relName field in the record to newName
	  strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal,newName);
    //    set back the record using RecBuffer.setRecord
	  attrCatBuffer.setRecord(attrRecord,recId.slot);
    }
    return SUCCESS;
}
int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal,relName);
    // Search for the relation with name relName in relation catalog using linearSearch()
    RecId recId=BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    // If relation with name relName does not exist (search returns {-1,-1})
    if(recId.block==-1 && recId.slot==-1)
    {
	return E_RELNOTEXIST;
    }

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    // declare variable attrToRenameRecId used to store the attr-cat recId
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrRecId;
    attrRecId.block=-1;
    attrRecId.slot=-1;
    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
	recId=BlockAccess::linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);
        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        if(recId.block==-1 && recId.slot==-1)
	{
		break;
	}
	RecBuffer attrCatBuffer(recId.block);
	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
	attrCatBuffer.getRecord(attrCatRecord,recId.slot);
        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
	if(strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0)
        {
             return E_ATTREXIST;
	}
	if(strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)==0)
	{
		attrRecId.block=recId.block;
		attrRecId.slot=recId.slot;
	}
    }
    if(attrRecId.slot==-1 && attrRecId.block==-1)
    {
	 return E_ATTRNOTEXIST;
    }
    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord
    else
    {
	RecBuffer attrCatBuffer(attrRecId.block);
	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
	attrCatBuffer.getRecord(attrCatRecord,attrRecId.slot);
	strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
	attrCatBuffer.setRecord(attrCatRecord,attrRecId.slot);
    }

    return SUCCESS;
}
int BlockAccess::insert(int relId, Attribute *record) {
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    int blockNum = relCatEntry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1;

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
	RecBuffer recBlock(blockNum);
        // get header of block(blockNum) using RecBuffer::getHeader() function
	HeadInfo head;
	recBlock.getHeader(&head);
        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
	unsigned char slotMap[numOfSlots];
	recBlock.getSlotMap(slotMap);
        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
	for(int i=0;i<numOfSlots;i++)
	{
		if(slotMap[i]==SLOT_UNOCCUPIED)
		{
			rec_id.block=blockNum;
			rec_id.slot=i;
			break;
		}
	}
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */

        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */
	if(rec_id.block!=-1)
	{
		break;
	}
        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
	prevBlockNum=blockNum;
	blockNum=head.rblock;
    }

    if(rec_id.block==-1 && rec_id.slot==-1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
	if(relId==RELCAT_RELID)
	{
		return E_MAXRELATIONS;
	}
        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
	RecBuffer newBlock;
	int ret=newBlock.getBlockNum();
        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }
        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
	rec_id.block=ret;
	rec_id.slot=0;
        /*
            set the header of the new record block such that it links with
            existing record blocks of the relation
            set the block's header as follows:
            blockType: REC, pblock: -1
            lblock
                  = -1 (if linked list of existing record blocks was empty
                         i.e this is the first insertion into the relation)
                  = prevBlockNum (otherwise),
            rblock: -1, numEntries: 0,
            numSlots: numOfSlots, numAttrs: numOfAttributes
            (use BlockBuffer::setHeader() function)
        */
	HeadInfo header;
	header.blockType=REC;
	header.numAttrs=numOfAttributes;
	header.numSlots=numOfSlots;
	header.numEntries=0;
	header.rblock=-1;
	header.lblock=prevBlockNum;
	header.pblock=-1;


	newBlock.setHeader(&header);
        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
	unsigned char slotMap[numOfSlots];
	for(int i=0;i<numOfSlots;i++)
	{
		slotMap[i]=SLOT_UNOCCUPIED;
	}


	newBlock.setSlotMap(slotMap);
       if (prevBlockNum != -1)
        {
            // create a RecBuffer object for prevBlockNum
	    RecBuffer prevBlock(prevBlockNum);
	    HeadInfo prevhead;
            // get the header of the block prevBlockNum and
	    prevBlock.getHeader(&prevhead);
            // update the rblock field of the header to the new block
	    prevhead.rblock=rec_id.block;
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
	    prevBlock.setHeader(&prevhead);
        }
        else
        {
	    RelCatEntry relCatEntry;
	    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
	    relCatEntry.firstBlk=rec_id.block;
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
	    RelCacheTable::setRelCatEntry(relId,&relCatEntry);
        }
	RelCatEntry relCatEntry;
	RelCacheTable::getRelCatEntry(relId,&relCatEntry);
	relCatEntry.lastBlk=rec_id.block;
        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
	RelCacheTable::setRelCatEntry(relId,&relCatEntry);
    }
    RecBuffer recBlock(rec_id.block);
    // create a RecBuffer object for rec_id.block
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    recBlock.setRecord(record,rec_id.slot);
    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    unsigned char slotMap[numOfSlots];
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
    recBlock.getSlotMap(slotMap);
    slotMap[rec_id.slot]=SLOT_OCCUPIED;
    recBlock.setSlotMap(slotMap);
    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    HeadInfo header;
    recBlock.getHeader(&header);
    (header.numEntries)++;
    recBlock.setHeader(&header);
    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    (relCatEntry.numRecs)++;
    RelCacheTable::setRelCatEntry(relId,&relCatEntry);
    return SUCCESS;
}
/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId;
    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */
    recId=linearSearch(relId,attrName,attrVal,op);
    // if there's no record satisfying the given condition (recId = {-1, -1})
    if(recId.block == -1 || recId.slot == -1)
    {
    	return E_NOTFOUND;
    }
    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recBlockBuffer(recId.block);
    recBlockBuffer.getRecord(record,recId.slot);
    return SUCCESS;
}
int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
    if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0)
    {
	return E_NOTPERMITTED;
    }
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    strcpy(relNameAttr.sVal,relName);
    RecId recId;
    //  linearSearch on the relation catalog for RelName = relNameAttr
    recId=linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    // if the relation does not exist (linearSearch returned {-1, -1})
    if(recId.block == -1 && recId.slot == -1)
    {
         return E_RELNOTEXIST;
    }
    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
    RecBuffer relCatBuffer(recId.block);
    relCatBuffer.getRecord(relCatEntryRecord,recId.slot);
    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    int firstBlk=relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
    int numAttrs=relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /*
     Delete all the record blocks of the relation
    */
    int block=firstBlk;
    // for each record block of the relation:
    //     get block header using BlockBuffer.getHeader
    //     get the next block from the header (rblock)
    //     release the block using BlockBuffer.releaseBlock
    //
    //     Hint: to know if we reached the end, check if nextBlock = -1
    while(block != -1)
    {
	RecBuffer recBuffer(block);
	HeadInfo header;
	recBuffer.getHeader(&header);
	int nextblock=header.rblock;
	recBuffer.releaseBlock();
	block=nextblock;
    }

    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numberOfAttributesDeleted = 0;

    while(true) {
        RecId attrCatRecId;
         attrCatRecId = linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;
	if(attrCatRecId.block==-1 && attrCatRecId.slot==-1)
	{
		break;
	}
        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
	RecBuffer attrCatBuffer(attrCatRecId.block);
        // get the header of the block
	HeadInfo attrHead;
	attrCatBuffer.getHeader(&attrHead);
        // get the record corresponding to attrCatRecId.slot
	Attribute record[ATTRCAT_NO_ATTRS];
	attrCatBuffer.getRecord(record,attrCatRecId.slot);
        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
        int rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        // (This will be used later to delete any indexes if it exists)
	unsigned char slotMap[attrHead.numSlots];
	attrCatBuffer.getSlotMap(slotMap);
        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
	slotMap[attrCatRecId.slot]=SLOT_UNOCCUPIED;
	attrCatBuffer.setSlotMap(slotMap);
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap

        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */
	attrHead.numEntries--;
	attrCatBuffer.setHeader(&attrHead);

        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        if(attrHead.numEntries == 0) {
            /* Standard Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock
            */

            // create a RecBuffer for lblock and call appropriate methods
	    if(attrHead.lblock != -1)
	    {
		RecBuffer leftBlock(attrHead.lblock);
		HeadInfo leftHead;
		leftBlock.getHeader(&leftHead);
		leftHead.rblock=attrHead.rblock;
		leftBlock.setHeader(&leftHead);
	    }
            if ( attrHead.rblock != -1 ) {
		RecBuffer rightBlock(attrHead.rblock);
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
		HeadInfo rightHead;
		rightBlock.getHeader(&rightHead);
		rightHead.lblock=attrHead.lblock;
		rightBlock.setHeader(&rightHead);
                // create a RecBuffer for rblock and call appropriate methods

            } else {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
		relCatEntryRecord[RELCAT_LAST_BLOCK_INDEX].nVal=attrHead.lblock;
		attrCatBuffer.setRecord(record,recId.slot);
            }
            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)
            // call releaseBlock()
	    attrCatBuffer.releaseBlock();
        }

        // (the following part is only relevant once indexing has been implemented)
        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        if (rootBlock != -1) {
            // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
        }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block
    HeadInfo relHeader;
    relCatBuffer.getHeader(&relHeader);
    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */
    relHeader.numEntries--;
    relCatBuffer.setHeader(&relHeader);
    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */
    unsigned char slotMap[relHeader.numSlots];
    relCatBuffer.getSlotMap(slotMap);
    slotMap[recId.slot]=SLOT_UNOCCUPIED;
    relCatBuffer.setSlotMap(slotMap);
    /*** Updating the Relation Cache Table ***/
    RelCatEntry relEntry;
    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    RelCacheTable::getRelCatEntry(RELCAT_RELID,&relEntry);
    relEntry.numRecs--;
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCacheTable::setRelCatEntry(RELCAT_RELID,&relEntry);
    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted
    // Get the entry corresponding to attribute catalog from the relation
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relEntry);
    relEntry.numRecs=relEntry.numRecs-numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID,&relEntry);
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)

    return SUCCESS;
}
