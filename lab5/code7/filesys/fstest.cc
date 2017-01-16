// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"
#include "directory.h"
#include "synchconsole.h"

#define TransferSize 	10 	// make it small, just to be difficult
#define DirectoryFileSize (10*sizeof(DirectoryEntry))
//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }
    
    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;		// close the Nachos file
    return;
}
//----------------------------------------------------------------------
// Copy
//  Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------
void
Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {   
    printf("Copy: couldn't open input file %s\n", from);
    return;
    }
// Figure out length of UNIX file
    fseek(fp, 0, 2);        
    fileLength = ftell(fp);
    fseek(fp, 0, 0);
// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, 0,0)) {  // Create Nachos file
    printf("Copy: couldn't create output file %s\n", to);
    fclose(fp);
    return;
    }
    //printf("I AM HERE\n");
    //return;
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);
    
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
    openFile->Write(buffer, amountRead);    
    //delete [] buffer;
   //  printf("lowIndex i: %d\n",lowIndex[i]);
// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
   // Print(to);
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"/hello"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize * 5000))

static void 
FileWrite()
{
    OpenFile *openFile;    
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
    if (!fileSystem->Create(FileName, 0,0)) {
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
	printf("Perf test: unable to open %s\n", FileName);
	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
	if (numBytes < 10) {
	    printf("Perf test: unable to write %s\n", FileName);
	    delete openFile;
	    return;
	}
    }
    delete openFile;	// close file
}

static void 
FileRead()
{
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL) {
	printf("Perf test: unable to open file %s\n", FileName);
	delete [] buffer;
	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
	if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
	    printf("Perf test: unable to read %s\n", FileName);
	    delete openFile;
	    delete [] buffer;
	    return;
	}
    }
    delete [] buffer;
    delete openFile;	// close file
}

void
PerformanceTest()
{
    printf("Starting file system performance test:\n");
    stats->Print();
    FileWrite();
    FileRead();
    if (!fileSystem->Remove(FileName)) {
      printf("Perf test: unable to remove %s\n", FileName);
      return;
    }
    stats->Print();

}
char testContentIni[2]="0";
char writeContent[2]="1";
void WrapRead(char* MyName)
{
    OpenFile *openFile;    
    openFile = fileSystem->Open(MyName) ; 
    char buffer[2]; 
    for (int i = 0; i < 3; i++) 
    {openFile->Read(buffer, 1);
     printf("%s READ %d th Byte as %s\n",currentThread->getName(),i,buffer);
     currentThread->Yield();
    }
    delete openFile;
    /*openFile = fileSystem->Open(MyName) ; 
    for (int i = 0; i < 1; i++) 
    {openFile->Read(buffer, 1);
     printf("%s READ %d th Byte as %s\n",currentThread->getName(),i,buffer);
    currentThread->Yield();
    }
    delete openFile;*/
}
void WrapWrite(char *MyName)
{
    OpenFile *openFile = fileSystem->Open(MyName);
    for(int i=0;i<3;i++)
    {
        printf("%s Write %d th Byte as %s\n",currentThread->getName(),i,writeContent);
        openFile->Write(writeContent,1);
        currentThread->Yield();
    }
    delete openFile;
}
void testForRemove(char *MyName)
{
    fileSystem->Remove(MyName);
    return;
}
void TestForDirectEX(char* from ,char * to)
{
      
    fileSystem->Create(to,0,0);// new a file
    OpenFile * iniFile = fileSystem->Open(to);// open the file for ini 
    for(int i=0;i<3;i++)
        iniFile->Write(testContentIni, 1);
    printf("ini---------------\n");
    iniFile->PrintFile();
    printf("ini---------------\n");
    delete iniFile;
    Thread * t1 = Thread::CreateThread("t1");
    Thread * t2 = Thread::CreateThread("t2");
    Thread * t3 = Thread::CreateThread("t3");
    t1->Fork(WrapRead,to);
    t3->Fork(testForRemove,to);
    t2->Fork(WrapWrite,to);
}    
    //WrapRead(to);

    //wrapRead(0);

    /*char testdirect[20]="/hello";
    if (!fileSystem->Create(testdirect, DirectoryFileSize,1)) {     // Create Nachos file
    printf("Copy: couldn't create output file %s\n", testdirect);
    return;
    } 
    char seconddirect[30]="/hello/world";
    if (!fileSystem->Create(seconddirect, DirectoryFileSize,1)) {     // Create Nachos file
    printf("Copy: couldn't create output file %s\n", seconddirect);
    return;
    } 
    char finalfile[30]="/hello/world/GJY";
    char Linuxfile[20]="directory.cc";
    Copy(Linuxfile,finalfile);
    printf("Copy from %s to %s successfully\n",Linuxfile,finalfile);
    fileSystem->List();
    if(!fileSystem->Remove(finalfile))
    {
        printf("can not delete %s\n",finalfile);
        return ;
    }
    printf("successfully delete %s\n",finalfile);
    fileSystem->List();*/
void TestForConsole(char *in, char *out)
{
    char ch;
    SynchConsole * console = new SynchConsole(in ,out);
    for (;;) {
    ch = console->Rchar();
    console->Wchar(ch);   // echo it!
    if (ch == 'q') break;  // if q, quit
    }
    delete console;
    return ;
}