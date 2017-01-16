// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
    {   // printf("I AM HERE in FindIndex\n");
         //printf("%d\n",table[i].inUse);
       // printf("I AM HERE\n");
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
	    return i;
    }
    return -1;		// name not in directory
}

DirectoryEntry  // find a file's directoryentry by paths
Directory::FindByPath(char * path)
{
    ASSERT(path[0]=='/');
    char temp[50];
    int length = strlen(path);
    int pos=0;
    for(pos=1;pos<length&&path[pos]!='/';pos++);  
    strncpy(temp,path+1,pos-1);
    temp[pos-1]='\0';
    int id = FindIndex(temp);
    if(id==-1)
    {
        printf("no such file\n");
        ASSERT(FALSE);
    }
    if(pos==length) // find a normal file
    {
        //printf("find file in %d th position in the Dir\n",id);
        return table[id];
    }
    else
    {
        ASSERT(table[id].type==1);
        Directory *directory = new Directory(10);
        OpenFile* dirfile = new OpenFile(table[id].sector); // find a path    
        directory->FetchFrom(dirfile); // load the dir
        strncpy(temp,path+pos,length-pos);
        temp[length-pos]='\0';
        DirectoryEntry returnresult = directory->FindByPath(temp);
        delete dirfile;
        delete directory;
        return returnresult;
    }    
}
//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector,int filetype)
{ 
    ASSERT(name[0]=='/');
    int pos=0;
    int length = strlen(name);
    char temp[50];
    for(pos=1;name[pos]!='/'&&pos<length;pos++);
    strncpy(temp,name+1,pos-1);
    temp[pos-1]='\0'; 
    if(pos<length)
    {
        int id = FindIndex(temp);// find the dir
        ASSERT(id!=-1);
        ASSERT(table[id].type==1);// it must be a dir;
        Directory *directory = new Directory(10);
        OpenFile* dirfile = new OpenFile(table[id].sector); // find a path    
        directory->FetchFrom(dirfile); // load the dir
        bool returnresult = directory->Add(name+pos,newSector,filetype);
        directory->WriteBack(dirfile);
        delete dirfile;
        delete directory;
        return returnresult;
    }
    else //find the final place to add;
    {

        if(FindIndex(temp)!=-1)
            return FALSE;
        for(int i=0;i< tableSize;i++)
            if (!table[i].inUse) 
            {
                table[i].inUse = TRUE;
                //strncpy(table[i].name, name, FileNameMaxLen); 
                table[i].name=name+1;
                table[i].type=filetype;// type = 0 means normal file
                table[i].sector = newSector;
                return TRUE;
            }
        return FALSE;    
    }
    /*if (FindIndex(name) != -1)
	return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            //strncpy(table[i].name, name, FileNameMaxLen); 
            table[i].name=name;
            table[i].type=filetype;// type = 0 means normal file
            table[i].sector = newSector;
        return TRUE;
	}
    return FALSE;	// no space.  Fix when we have extensible files.
    */
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    ASSERT(name[0]=='/');
    int pos=0;
    char temp[50];
    int length = strlen(name);
    for(pos=1;pos<length&&name[pos]!='/';pos++);
    strncpy(temp,name+1,pos-1);
    temp[pos-1]='\0';    
    if(pos==length) // find the final name
    {
        int i = FindIndex(temp);
        if(i==-1)
        {
            printf("no such file to delete\n");
            return FALSE;
        }
        if(table[i].type==1)// it is a directory
        {
          Directory * tempdirect = new Directory(10);
          OpenFile * tempfile = new OpenFile(table[i].sector);
          tempdirect->FetchFrom(tempfile);
          for(int j=0;j<10;j++)
          {
            char c[50]="/";
            if(tempdirect->table[j].inUse)
                {
                    strcat(c,table[j].name);
                    //printf("try to delete %s\n",c);
                    bool deleteresult=tempdirect->Remove(c);
                    if(!deleteresult)
                        return FALSE;
                }
          }             
        }
        BitMap *freeMap =new BitMap(NumSectors) ;
        freeMap->FetchFrom(fileSystem->freeMapFile); // get the BitMap
        FileHeader *fileHdr = new FileHeader;
        fileHdr->FetchFrom(table[i].sector); // get the header
        fileHdr->Deallocate(freeMap); // deallocate the content;
        freeMap->Clear(table[i].sector);  // deallocate the header  
        table[i].inUse = FALSE; // disable the entry
        freeMap->WriteBack(fileSystem->freeMapFile);
        delete freeMap;
        delete fileHdr;
        return TRUE;
    } 
    else // haven't find the final file
    {
        int id = FindIndex(temp);
        ASSERT(id!=-1)
        ASSERT(table[id].type==1);
        Directory * tempdirect = new Directory(10);
        OpenFile * tempfile = new OpenFile(table[id].sector);
        tempdirect->FetchFrom(tempfile);
        bool finalresult=tempdirect->Remove(name+pos);
        tempdirect->WriteBack(tempfile);
        delete tempdirect;
        delete tempfile;
        return finalresult;
    }   
    /*int i = FindIndex(name);

    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	*/
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse)
	    {
            printf("%s ", table[i].name);
            if(table[i].type==1)
            {
                printf(":");
                Directory * tempdirect = new Directory(10);
                OpenFile * tempfile = new OpenFile(table[i].sector);
                tempdirect->FetchFrom(tempfile);
                tempdirect->List();
                printf("\n");
                delete tempdirect;
                delete tempfile;
            }
        }    
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}

