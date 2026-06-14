#include "OpenRelTable.h"

#include <cstring>

#include <cstdlib>

#include <iostream>
//tableMetaInfo is a struct with relName and free as datas
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free=true;
    tableMetaInfo[i].relName[0]='\0';
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
  relCacheEntry.searchIndex.block=-1;
  relCacheEntry.searchIndex.slot=-1;
  relCacheEntry.dirty=false;
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
  attrRelCacheEntry.searchIndex.block=-1;
  attrRelCacheEntry.searchIndex.slot=-1;
  attrRelCacheEntry.dirty=false;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrRelCacheEntry;

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
	temp->dirty=false;
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
	temp->dirty=false;
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
  AttrCacheTable::attrCache[ATTRCAT_RELID]=head;
  //updating the meta-data of the RELCAT and ATTRCAT
  tableMetaInfo[RELCAT_RELID].free=false;
  tableMetaInfo[ATTRCAT_RELID].free=false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);
}
int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
  int relId=OpenRelTable::getRelId(relName);
  if(relId != E_RELNOTOPEN){
    // (checked using OpenRelTable::getRelId())
    return relId;
    // return that relation id;
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
  int freeslot=OpenRelTable::getFreeOpenRelTableEntry();
  if (freeslot == E_CACHEFULL){
    return E_CACHEFULL;
  }

  // let relId be used to store the free slot.
  relId=freeslot;

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/
  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  Attribute relNameAttr;
  //Linear search only accept attribute value  to be in the union attribute datatype
  strcpy((char*)relNameAttr.sVal,(char*)relName);
  RecId recId;
  //recId has the block and slotNo of the record in relcat with the inputed relname.
  recId=BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,relNameAttr,EQ);
  int block=recId.block;
  int slot=recId.slot;
  //std::cout<<block<<" "<<slot<<"\n";
  if(block==-1 || slot==-1)
  {
	return E_RELNOTEXIST;
  }
  //retreiving the relcat of the relation with the inputed relName
  RecBuffer relCatBlock(block);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord,slot);

  //storing the retrieved relCat of that relation in a newly created RelCacheEntry and pushing it to relCache onto an index which is free
  RelCacheEntry *relCacheEntry=(RelCacheEntry *)(malloc(sizeof(RelCacheEntry)));
  RelCacheTable::recordToRelCatEntry(relCatRecord,&(relCacheEntry->relCatEntry));
  relCacheEntry->recId=recId;
  relCacheEntry->dirty=false;
  relCacheEntry->searchIndex.block=-1;
  relCacheEntry->searchIndex.slot=-1;

  RelCacheTable::relCache[relId]=relCacheEntry;

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  //Initialising the attrCache[relId] with the AttrCacheEntry of the inputted relation.
  AttrCacheEntry* listHead=NULL;
  AttrCacheEntry* prev=NULL;
  //Resetting search_index so that all the records would be read properly...
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  while(true)
  {
	Attribute attrRelName;
	strcpy((char *)attrRelName.sVal,(char *)relName);
 	//Linear Search on AttrCatTable to find the block no and slot no of  attributes belonging to the relation with the name relName;
	RecId attrId=BlockAccess::linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,attrRelName,EQ);
	//block==-1 or slot ==-1 when all the matching records have been retrieved.
	if(attrId.block==-1 || attrId.slot==-1)
	{
		break;
	}
	//retrieving the records from ATTRCAT using the block and slot index from above.
	Attribute attrRecord[ATTRCAT_NO_ATTRS];
	RecBuffer attrCatBlock(attrId.block);
	attrCatBlock.getRecord(attrRecord,attrId.slot);
	//creating an AttrCacheEntry to store the AttrCat of all attributes of relName
	AttrCacheEntry *attrCacheEntry=(AttrCacheEntry *)(malloc(sizeof(AttrCacheEntry)));
	AttrCacheTable::recordToAttrCatEntry(attrRecord,&(attrCacheEntry->attrCatEntry));

	attrCacheEntry->recId=attrId;
	attrCacheEntry->dirty=false;
	attrCacheEntry->next=NULL;
	//For a relation with name relName..all of its attribute cache entries will be on a SLL on attrCache[relId]
	if(listHead==NULL)
	{
		listHead=attrCacheEntry;
		prev=attrCacheEntry;
	}
	else
	{
		prev->next=attrCacheEntry;
		prev=attrCacheEntry;
	}
  }
  AttrCacheTable::attrCache[relId]=listHead;

  //Updating the metadata of this relation with the generated relId.
  tableMetaInfo[relId].free=false;
  strcpy((char *)tableMetaInfo[relId].relName,(char *)relName);

  return relId;
}

int OpenRelTable::closeRel(int relId) {
  //Involves freeing all the pointers created in OpenRelTable::openRel().
  if (relId==RELCAT_RELID || relId==ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }

  if (relId<0 || relId>=MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free==true) {
    return E_RELNOTOPEN;
  }
  free(RelCacheTable::relCache[relId]);
  RelCacheTable::relCache[relId]=NULL;
  AttrCacheEntry* temp=AttrCacheTable::attrCache[relId];
  while(temp!=NULL)
  {
	AttrCacheEntry* freetemp=temp;
	temp=temp->next;
	free(freetemp);
	freetemp=NULL;
  }
  AttrCacheTable::attrCache[relId]=NULL;
  (tableMetaInfo[relId].relName)[0]='\0';
  tableMetaInfo[relId].free=true;
  return SUCCESS;
}
int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{
	int i=0;
	while(i<MAX_OPEN && strcmp(tableMetaInfo[i].relName,relName)!=0)
	{
		i++;
	}
	if(i==MAX_OPEN)
	{
		return E_RELNOTOPEN;
	}
	return i;
}
int OpenRelTable::getFreeOpenRelTableEntry()
{
	int i=0;
	while(i<MAX_OPEN&&tableMetaInfo[i].free==false)
	{
		i++;
	}
	if(i==MAX_OPEN)
	{
		return E_CACHEFULL;
	}
	return i;
}
OpenRelTable::~OpenRelTable() {
  for(int i=0;i<MAX_OPEN;i++)
  {
	if(RelCacheTable::relCache[i]!=NULL)
	{
		free(RelCacheTable::relCache[i]);
		RelCacheTable::relCache[i]=NULL;
		tableMetaInfo[i].free=true;
		tableMetaInfo[i].relName[0]='\0';
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
