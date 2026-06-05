#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
  RecBuffer attrCatBuffer1(ATTRCAT_BLOCK+1);
  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;
  HeadInfo attrCatHeader1;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);
  attrCatBuffer1.getHeader(&attrCatHeader1);

  for (int i=0;i<relCatHeader.numEntries;i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    for (int j=0;j<attrCatHeader.numEntries;j++) {

      // declare attrCatRecord and load the attribute catalog entry into it
	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
	attrCatBuffer.getRecord(attrCatRecord,j);
      if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal)==0) {

	if(strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,"Students")==0 && strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Class")==0)
	{
		strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Batch");
		attrCatBuffer.setRecord(attrCatRecord,j);
	}
	const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        printf("  %s: %s\n",attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
      }
    }
    for (int j=0;j<attrCatHeader1.numEntries;j++) {

      // declare attrCatRecord and load the attribute catalog entry into it
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer1.getRecord(attrCatRecord,j);
      if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal)==0) {

	if(strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,"Student")==0 && strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Class")==0)
        {
                strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Batch");
                attrCatBuffer1.setRecord(attrCatRecord,j);
        }
        const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        printf("  %s: %s\n",attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
      }
    }
    printf("\n");
  }
  return 0;
}
