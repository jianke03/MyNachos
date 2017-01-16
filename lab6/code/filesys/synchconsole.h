// synchconsole.h 

#include "copyright.h"

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "console.h"
#include "synch.h"

class SynchConsole {
  public:
    SynchConsole(char *readFile, char *writeFile);    		// Initialize a synchronous disk,
					// by initializing the raw Disk.
    ~SynchConsole();			// De-allocate the synch disk data
    
    //void ReadSector(int sectorNumber, char* data);
    					// Read/write a disk sector, returning
    					// only once the data is actually read 
					// or written.  These call
    					// Disk::ReadRequest/WriteRequest and
					// then wait until the request is done.
    void Wchar(char ch);//(int sectorNumber, char* data);
    
    char Rchar();			// Called by the disk device interrupt
					// handler, to signal that the
					// current disk operation is complete.
    void WriteDone();
    void ReadDone();
  private:
    Console *console;		  		// Raw disk device
    Semaphore *Readsemaphore; 		// To synchronize requesting thread 				// with the interrupt handler
    Semaphore *Writesemaphore;
    Lock *lock;		  		// Only one read/write request
					// can be sent to the disk at a time
};

#endif // SYNCHCONSOLE_H
