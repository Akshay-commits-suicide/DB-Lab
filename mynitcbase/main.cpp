#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer bufferr;
  OpenRelTable cache;
  /*for(int i=0;i<3;i++)
  {
	RelCatEntry relCatEntry;
	int a=RelCacheTable::getRelCatEntry(i,&relCatEntry);
	printf("Relation: %s\n",relCatEntry.relName);
	for(int j=0;j<relCatEntry.numAttrs;j++)
	{
		AttrCatEntry attrCatEntry;
		int b=AttrCacheTable::getAttrCatEntry(i,j,&attrCatEntry);
		const char* type= attrCatEntry.attrType == NUMBER ? "NUM" : "STR" ;
		printf("  %s:%s\n",attrCatEntry.attrName,type);
	}
  }*/
  return FrontendInterface::handleFrontend(argc,argv);
}
