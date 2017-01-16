// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include <time.h>

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize); //needed sector numbers
    if (freeMap->NumClear() < numSectors)
	return FALSE;
    int copyNumSectors = numSectors;		// not enough space
    int temp = min(copyNumSectors,9);
    for(int i=0;i<temp;i++)
        {
            dataSectors[i] = freeMap->Find();
            if(dataSectors[i]==-1)
                ASSERT(FALSE);
        }
    copyNumSectors-=temp;    
    if(copyNumSectors>0)
    {
        dataSectors[9]=freeMap->Find();
        int firstIndex[32];
        temp = min(32,copyNumSectors);
        for(int i=0;i<temp;i++)
        {
            firstIndex[i] = freeMap->Find();
            if(firstIndex[i]==-1)
                ASSERT(FALSE);
        }
        copyNumSectors-=temp;
        synchDisk->WriteSector(dataSectors[9], (char*)firstIndex);
        if(copyNumSectors>0) // need two-level index
        {
            dataSectors[10] = freeMap->Find(); //save 2-level index
            if(dataSectors[10]==-1)
                ASSERT(FALSE);
            int id=0;
            int secondIndex[32];
            while(copyNumSectors>0)
            {
                secondIndex[id] = freeMap->Find();
                if(secondIndex[id]==-1)
                    ASSERT(FALSE);
                int lowIndex[32];
                temp = min(32,copyNumSectors);
                for(int i=0;i<temp;i++)
                {
                  
                    lowIndex[i]= freeMap->Find();
                    if(lowIndex[i]==-1)
                        ASSERT(FALSE);
                }
               synchDisk->WriteSector(secondIndex[id], (char*)lowIndex);
               id+=1;
               copyNumSectors-=temp; 
            }
            synchDisk->WriteSector(dataSectors[10], (char*)secondIndex);
        }
    }    

    {
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char* tempTime = asctime(timeinfo);
        strncpy(createTime,tempTime,24);
        strncpy(lastVisitTime,tempTime,24);
        strncpy(lastModifyTime,tempTime,24);
       //   printf("file created at %s\n",createTime);
    }  
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    /*for (int i = 0; i < numSectors; i++) {
	ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	freeMap->Clear((int) dataSectors[i]);  
    }*/
    //printf("I AM IN DE\n");
    int copyNumSectors = numSectors;
    int temp = min(numSectors,9);
    for(int i=0;i<temp;i++)
    {
      ASSERT(freeMap->Test((int) dataSectors[i]));
      freeMap->Clear((int)dataSectors[i]);   
    }
    copyNumSectors-= temp;
    if(copyNumSectors>0)
    {
        temp = min(copyNumSectors,32);
        int firstIndex[32];
        synchDisk->ReadSector(dataSectors[9], (char *)firstIndex);
        for(int i=0;i<temp;i++)
            {
                ASSERT(freeMap->Test((int) firstIndex[i]));
                freeMap->Clear((int)firstIndex[i]);
            }
        ASSERT(freeMap->Test((int) dataSectors[9]));
        freeMap->Clear((int)dataSectors[9]); 
        copyNumSectors-=temp;
        if(copyNumSectors>0)
        {
            int secondIndex[32];
            synchDisk->ReadSector(dataSectors[10],(char*)secondIndex);
            int id=0;
            while(copyNumSectors>0)
            {
                int lowIndex[32];
                synchDisk->ReadSector(secondIndex[id],(char*)lowIndex);
                temp = min(32,copyNumSectors);
                for(int i=0;i<temp;i++)
                {
                    ASSERT(freeMap->Test((int) lowIndex[i]));
                    freeMap->Clear((int)lowIndex[i]);
                }
                freeMap->Clear((int)secondIndex[id]);
                id+=1;
                copyNumSectors-=temp;
            }
            ASSERT(freeMap->Test((int)dataSectors[10]));
            freeMap->Clear((int)dataSectors[10]);
        }  
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    //printf("I AM HERE %d\n",sector);
    //printf("%d\n",sizeof(FileHeader));
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int speSector = offset/SectorSize;
    if(speSector<9)
    {
        return dataSectors[speSector];
    }
    speSector-=9;
    if(speSector<32)
    {
        int firstIndex[32];
        synchDisk->ReadSector(dataSectors[9],(char*)firstIndex);
        return firstIndex[speSector];
    }
    else
    {   speSector-=32;
        if(speSector>=0)
        {
            int id = speSector/32;
            int secondIndex[32];
            synchDisk->ReadSector(dataSectors[10],(char*)secondIndex);
           
            int lowIndex[32];
            int offsetinlow = speSector%32;
            synchDisk->ReadSector(secondIndex[id],(char*)lowIndex);
            return lowIndex[offsetinlow];
        }
    }   
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];
    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
       // printf("I AM HERE\n");
	synchDisk->ReadSector(dataSectors[i], data);

        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}

void 
FileHeader::PrintTime()
{
    printf("create Time: %s\n",createTime);
    printf("modify Time: %s\n",lastModifyTime);
    printf("visit Time: %s\n",lastVisitTime);
}

bool
FileHeader::ExtraAllocate(BitMap * bitMap, int addtionalSize)
{
    int totalSize = numBytes + addtionalSize;
    int totalSectors  = divRoundUp(totalSize, SectorSize);
    numBytes = totalSize;
    if(totalSectors == numSectors) // now the size is enough
        return TRUE;  
    else
    {
        if(bitMap->NumClear()<(totalSectors-numSectors))
        {printf("no enough space for extra allocate\n");return FALSE;};
        if(numSectors<=9)
        {
            int copyNumSectors = totalSectors-numSectors;
            int temp = min(copyNumSectors,9-numSectors);
            for(int i=0;i<temp;i++)
                {
                    dataSectors[numSectors+i] = bitMap->Find();
                    ASSERT(dataSectors[numSectors+i]!=-1);
                }
            copyNumSectors-=temp;    
            if(copyNumSectors>0)
            {
                dataSectors[9]=bitMap->Find();
                int firstIndex[32];
                temp = min(32,copyNumSectors);
                for(int i=0;i<temp;i++)
                {
                    firstIndex[i] = bitMap->Find();
                     ASSERT(firstIndex[i]!=-1);
                }
                copyNumSectors-=temp;
                synchDisk->WriteSector(dataSectors[9], (char*)firstIndex);
                if(copyNumSectors>0) // need two-level index
                {
                    dataSectors[10] = bitMap->Find(); //save 2-level index
                    ASSERT(dataSectors[10]!=-1);
                    int id=0;
                    int secondIndex[32];
                    while(copyNumSectors>0)
                    {
                        secondIndex[id] = bitMap->Find();
                        ASSERT(secondIndex[id]!=-1);
                        int lowIndex[32];
                        temp = min(32,copyNumSectors);
                        for(int i=0;i<temp;i++)
                        {
                          
                            lowIndex[i]= bitMap->Find();
                            ASSERT(lowIndex[i]!=-1)
                        }
                       synchDisk->WriteSector(secondIndex[id], (char*)lowIndex);
                       id+=1;
                       copyNumSectors-=temp; 
                    }
                    synchDisk->WriteSector(dataSectors[10], (char*)secondIndex);
                }
            } 
            numSectors = totalSectors; 
            return TRUE;  
        }
        else if(numSectors<=(32+9)) // first index still have space
        {
            int copyNumSectors = totalSectors-numSectors;
            int firstIndex[32];
            synchDisk->ReadSector(dataSectors[9],(char*)firstIndex);
            int firres = (41-numSectors);
            int temp = min(copyNumSectors,firres);

            for(int i=0;i<temp;i++)
            {
                firstIndex[numSectors-9+i] = bitMap->Find();
                ASSERT(firstIndex[numSectors-9+i])
            }
            copyNumSectors-=temp;
            synchDisk->WriteSector(dataSectors[9],(char*)firstIndex);
            if(copyNumSectors>0)
            {
                dataSectors[10] = bitMap->Find(); //save 2-level index
                ASSERT(dataSectors[10]!=-1);
                int id=0;
                int secondIndex[32];
                while(copyNumSectors>0)
                {
                    secondIndex[id] = bitMap->Find();
                    ASSERT(secondIndex[id]!=-1);
                    int lowIndex[32];
                    temp = min(32,copyNumSectors);
                    for(int i=0;i<temp;i++)
                    {
                          
                        lowIndex[i]= bitMap->Find();
                        ASSERT(lowIndex[i]!=-1)
                    }
                    synchDisk->WriteSector(secondIndex[id], (char*)lowIndex);
                    id+=1;
                    copyNumSectors-=temp; 
                }
                synchDisk->WriteSector(dataSectors[10], (char*)secondIndex); 
            }
            numSectors = totalSectors;
            return TRUE;
        }
        else //start from secondIndex;
        {
            int secondIndex[32];
            synchDisk->ReadSector(dataSectors[10],(char*)secondIndex);
            int copyNumSectors = totalSectors - numSectors;
            int In2sectors = numSectors - 41;// >0
            int id = In2sectors/32;
            int offset = In2sectors%32;
            if(offset!=0)
            {
                int lowIndex[32];
                synchDisk->ReadSector(secondIndex[id],(char*)lowIndex);
                int ressec = (32-offset);
                int temp = min(ressec,copyNumSectors);
                for(int i=0;i<temp;i++)
                {
                    lowIndex[offset+i]=bitMap->Find();
                    ASSERT(lowIndex[offset+i]!=-1);
                }
                synchDisk->WriteSector(secondIndex[id],(char*)lowIndex);
                id+=1;
                copyNumSectors-=temp; 
            }
            while(copyNumSectors>0)
            {
                secondIndex[id]= bitMap->Find();
                ASSERT(secondIndex[id]!=-1);
                int lowIndex[32];
                int temp = min(32,copyNumSectors);
                for(int i=0;i<temp;i++)
                {
                    lowIndex[i]= bitMap->Find();
                    ASSERT(lowIndex[i]!=-1);
                }
                synchDisk->WriteSector(secondIndex[id],(char*)lowIndex);
                copyNumSectors-=temp;
                id+=1;
            }
            synchDisk->WriteSector(dataSectors[10],(char*)secondIndex);
            numSectors = totalSectors;
            return TRUE; 
        }
    }
   
}