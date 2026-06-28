#include "Schema.h"

#include <cmath>
#include <cstring>
#include <cstdio>

int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);
  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  if(ret >= 0 && ret<MAX_OPEN){
    return SUCCESS;
  }

  //otherwise it returns an error message
  return ret;
}
int Schema::closeRel(char relName[ATTR_SIZE]) {
  if (strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0) {
    return E_NOTPERMITTED;
  }
  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId==E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }
  return OpenRelTable::closeRel(relId);
}
/*Rename functions are automatically opening and closing a relation so that the renamed column or relation name is effectively written back into the disk after the completition of
 that operation*/
int Schema::renameRel(char oldRelName[ATTR_SIZE],char newRelName[ATTR_SIZE])
{
	if(strcmp(oldRelName,RELCAT_RELNAME)==0 || strcmp(newRelName,RELCAT_RELNAME)==0 || strcmp(oldRelName,ATTRCAT_RELNAME)==0 || strcmp(newRelName,ATTRCAT_RELNAME)==0)
	{
		return E_NOTPERMITTED;
	}
	int ret=-1;
	if(OpenRelTable::getRelId(oldRelName) == E_RELNOTOPEN)
	{
		ret=OpenRelTable::openRel(oldRelName);
		if(ret == E_CACHEFULL || ret == E_RELNOTEXIST)
		{
			return ret;
		}
	}
	int retVal=BlockAccess::renameRelation(oldRelName,newRelName);
	int ret1=OpenRelTable::closeRel(ret);
	return retVal;
}
int Schema::renameAttr(char *relName,char oldAttrName[ATTR_SIZE],char newAttrName[ATTR_SIZE])
{
        if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0)
        {
                return E_NOTPERMITTED;
        }
	int ret=-1;
        if(OpenRelTable::getRelId(relName) == E_RELNOTOPEN)
        {
                ret=OpenRelTable::openRel(relName);
                if(ret == E_CACHEFULL || ret == E_RELNOTEXIST)
                {
                        return ret;
                }
        }
        int retVal=BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);
	int ret1=OpenRelTable::closeRel(ret);
        return retVal;
}
int Schema::deleteRel(char *relName) {
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    if(strcmp(relName,"RELATIONCAT")==0 || strcmp(relName,"ATTRIBUTECAT")==0)
    {
         return E_NOTPERMITTED;
    }
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)

    // get the rel-id using appropriate method of OpenRelTable class by
    // passing relation name as argument
    int relId=OpenRelTable::getRelId(relName);
    // if relation is not opened in open relation table, return E_RELNOTOPEN
    if(relId != E_RELNOTOPEN)
    {
	return E_RELOPEN;
    }
    // Call BlockAccess::deleteRelation() with appropriate argument.
    int ret=BlockAccess::deleteRelation(relName);
    // return the value returned by the above deleteRelation() call
    return ret;
    /* the only error that should be returned from deleteRelation() is E_RELNOTEXIST.
       The deleteRelation call may return E_OUTOFBOUND from the call to
       loadBlockAndGetBufferPtr, but if your implementation so far has been
       correct, it should not reach that point. That error could only occur
       if the BlockBuffer was initialized with an invalid block number.
    */
}
int Schema::createRel(char relName[],int nAttrs, char attrs[][ATTR_SIZE],int attrtype[]){

    // declare variable relNameAsAttribute of type Attribute
    Attribute relNameAsAttribute;
    // copy the relName into relNameAsAttribute.sVal
    strcpy(relNameAsAttribute.sVal,relName);
    // declare a variable targetRelId of type RecId
    RecId targetRelId;
    // Reset the searchIndex using RelCacheTable::resetSearhIndex()
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    // Search the relation catalog (relId given by the constant RELCAT_RELID)
    targetRelId=BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAsAttribute,EQ);
    // for attribute value attribute "RelName" = relNameAsAttribute using
    // BlockAccess::linearSearch() with OP = EQ

    // if a relation with name `relName` already exists  ( linearSearch() does
    //                                                     not return {-1,-1} )
    if(targetRelId.block !=-1 && targetRelId.slot !=-1)
    {
	 return E_RELEXIST;
    }
    // compare every pair of attributes of attrNames[] array
    // if any attribute names have same string value,
    //     return E_DUPLICATEATTR (i.e 2 attributes have same value)
    for(int i=0;i<nAttrs;i++)
    {
	for(int j=i+1;j<nAttrs;j++)
	{
		if(strcmp(attrs[i],attrs[j])==0)
		{
			return E_DUPLICATEATTR;
		}
	}
    }
    /* declare relCatRecord of type Attribute which will be used to store the
       record corresponding to the new relation which will be inserted
       into relation catalog */
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    // fill relCatRecord fields as given below
    // offset RELCAT_REL_NAME_INDEX: relName
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,relName);
    // offset RELCAT_NO_ATTRIBUTES_INDEX: numOfAttributes
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal=nAttrs;
    // offset RELCAT_NO_RECORDS_INDEX: 0
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal=0;
    // offset RELCAT_FIRST_BLOCK_INDEX: -1
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal=-1;
    // offset RELCAT_LAST_BLOCK_INDEX: -1
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal=-1;
    // offset RELCAT_NO_SLOTS_PER_BLOCK_INDEX: floor((2016 / (16 * nAttrs + 1)))
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal=floor(2016/(16*nAttrs+1));
    // (number of slots is calculated as specified in the physical layer docs)

    // retVal = BlockAccess::insert(RELCAT_RELID(=0), relCatRecord);
    int retVal = BlockAccess::insert(RELCAT_RELID,relCatRecord);
    // if BlockAccess::insert fails return retVal
    if(retVal != SUCCESS)
    {
	return retVal;
    }
    // (this call could fail if there is no more space in the relation catalog)

    // iterate through 0 to numOfAttributes - 1 :
    for(int i=0;i<nAttrs;i++)
    {
        /* declare Attribute attrCatRecord[6] to store the attribute catalog
           record corresponding to i'th attribute of the argument passed*/
	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        // (where i is the iterator of the loop)
        // fill attrCatRecord fields as given below
        // offset ATTRCAT_REL_NAME_INDEX: relName
	strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relName);
        // offset ATTRCAT_ATTR_NAME_INDEX: attrNames[i]
	strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrs[i]);
        // offset ATTRCAT_ATTR_TYPE_INDEX: attrTypes[i]
	attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal=attrtype[i];
        // offset ATTRCAT_PRIMARY_FLAG_INDEX: -1
	attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal=-1;
        // offset ATTRCAT_ROOT_BLOCK_INDEX: -1
	attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal=-1;
        // offset ATTRCAT_OFFSET_INDEX: i
	attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal=i;

        // retVal = BlockAccess::insert(ATTRCAT_RELID(=1), attrCatRecord);
	retVal = BlockAccess::insert(ATTRCAT_RELID,attrCatRecord);
	while(retVal!=SUCCESS)
	{
		Schema::deleteRel(relName);
		return E_DISKFULL;
	}
        /* if attribute catalog insert fails:
             delete the relation by calling deleteRel(targetrel) of schema layer
             return E_DISKFULL
             // (this is necessary because we had already created the
             //  relation catalog entry which needs to be removed)
        */
    }
    return SUCCESS;
    // return SUCCESS
}


