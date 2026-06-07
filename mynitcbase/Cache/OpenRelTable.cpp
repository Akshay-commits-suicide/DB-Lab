#include "OpenRelTable.h"

#include <cstring>

#include <cstdlib>

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]


  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/

  Attribute attrRelCatRecord[RELCAT_NO_ATTRS];

  relCatBlock.getRecord(attrRelCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);

  struct RelCacheEntry attrRelCacheEntry;
  RelCacheTable::recordToRelCatEntry(attrRelCatRecord, &attrRelCacheEntry.relCatEntry);
  attrRelCacheEntry.recId.block = RELCAT_BLOCK;
  attrRelCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrRelCacheEntry;

  //RelCacheEntry for Student
  Attribute StudentRelCatRecord[RELCAT_NO_ATTRS];

  relCatBlock.getRecord(StudentRelCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT+1);

  struct RelCacheEntry StudentRelCacheEntry;
  RelCacheTable::recordToRelCatEntry(StudentRelCatRecord, &StudentRelCacheEntry.relCatEntry);
  StudentRelCacheEntry.recId.block = RELCAT_BLOCK;
  StudentRelCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT+1;

  // allocate this on the heap because we want it to persist outside this funct>
  RelCacheTable::relCache[ATTRCAT_RELID+1] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID+1]) = StudentRelCacheEntry;

  // iterate through all the attributes of the relation catalog and create a linked
  // list of AttrCacheEntry (slots 0 to 5)
  // for each of the entries, set
  //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
  //    attrCacheEntry.recId.slot = i   (0 to 5)
  //    and attrCacheEntry.next appropriately
  // NOTE: allocate each entry dynamically using malloc
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);

  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  AttrCacheEntry* prev = NULL;
  AttrCacheEntry* head = NULL;
  for(int i=0;i<RELCAT_NO_ATTRS;i++)
  {
	attrCatBlock.getRecord(attrCatRecord,i);
	AttrCacheEntry* temp = (AttrCacheEntry*)(malloc(sizeof(AttrCacheEntry)));
	AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&temp->attrCatEntry);
	temp->recId.block=ATTRCAT_BLOCK;
	temp->recId.slot=i;
	temp->next=NULL;
	if(head==NULL)
	{
		head=temp;
	}
	else
	{
		prev->next=temp;
	}
	prev=temp;
  }
  AttrCacheTable::attrCache[RELCAT_RELID] = head;

  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  // set up the attributes of the attribute cache similarly.
  // read slots 6-11 from attrCatBlock and initialise recId appropriately
  prev=NULL;
  head=NULL;
  for(int i=6;i<12;i++)
  {
        attrCatBlock.getRecord(attrCatRecord,i);
        AttrCacheEntry* temp = (AttrCacheEntry*)(malloc(sizeof(AttrCacheEntry)));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&temp->attrCatEntry);
        temp->recId.block=ATTRCAT_BLOCK;
        temp->recId.slot=i;
        temp->next=NULL;
        if(head==NULL)
        {
                head=temp;
        }
        else
        {
                prev->next=temp;
        }
	prev=temp;
  }
  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
  AttrCacheTable::attrCache[ATTRCAT_RELID]=head;


//AttributeCacheEntries for Student Relation
  prev=NULL;
  head=NULL;
  for(int i=12;i<18;i++)
  {
        attrCatBlock.getRecord(attrCatRecord,i);
        AttrCacheEntry* temp = (AttrCacheEntry*)(malloc(sizeof(AttrCacheEntry)));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&temp->attrCatEntry);
        temp->recId.block=ATTRCAT_BLOCK;
        temp->recId.slot=i;
        temp->next=NULL;
        if(head==NULL)
        {
                head=temp;
        }
        else
        {
                prev->next=temp;
        }
        prev=temp;
  }
  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
  AttrCacheTable::attrCache[ATTRCAT_RELID+1]=head;
}
OpenRelTable::~OpenRelTable() {
  for(int i=0;i<MAX_OPEN;i++)
  {
	if(RelCacheTable::relCache[i]!=NULL)
	{
		free(RelCacheTable::relCache[i]);
		RelCacheTable::relCache[i]=NULL;
	}
  }
  for(int i=0;i<MAX_OPEN;i++)
  {
	AttrCacheEntry *curr=AttrCacheTable::attrCache[i];
        while(curr!=NULL)
        {
		AttrCacheEntry *temp=curr;
		curr=curr->next;
                free(temp);
        }
	AttrCacheTable::attrCache[i]=NULL;
  }

}
