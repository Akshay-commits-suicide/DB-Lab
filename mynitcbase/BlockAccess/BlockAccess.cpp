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
