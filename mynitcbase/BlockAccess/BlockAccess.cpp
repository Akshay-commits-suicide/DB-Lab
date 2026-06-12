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
    recId.block=block;
    recId.slot=slot;
    RelCacheTable::resetSearchIndex(relId);
    return recId;
}
