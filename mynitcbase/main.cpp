#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer,1);
  for(int i=0;i<20;i++)
  {
       std::cout<<(int)buffer[i]<<" ";
  }
  std::cout<<"\n";
  /*char message[]="Hello\n";
  memcpy(buffer+20,message,7);
  Disk::writeBlock(buffer,7000);
  unsigned char buffer2[BLOCK_SIZE];
  Disk::readBlock(buffer2,7000);
  char outmessage[7];
  memcpy(outmessage,buffer+20,7);
  std::cout<<outmessage;*/
  // StaticBuffer buffer;
  // OpenRelTable cache;
  return 0;
  //return FrontendInterface::handleFrontend(argc, argv);
}
