// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    for(int i=0;i<32;i++)
{
	readyList[i]=new List;
	numOfThreads[i]=0;
}
#ifdef USER_PROGRAM
suspendList= new List;
sleepsusList = new List;
#endif
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
   for(int i=0;i<32;i++)
	delete readyList[i];
#ifdef USER_PROGRAM
delete suspendList;
delete sleepsusList;
#endif
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());
    thread->timeslices=(currentThread->priority+1);
    thread->setStatus(READY);
    readyList[thread->priority]->Append((void *)thread);
    numOfThreads[thread->priority]+=1;
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
   int i=0;
   //printf("%d\n",numOfThreads[0]);
   while(numOfThreads[i]<=0&&i<31)
   	i++; 
   if (i==32)
     return NULL;
   numOfThreads[i]-=1;
   Thread* thread=(Thread *)readyList[i]->Remove();
   //if(thread!=NULL)
   //printf("thread %s is going to R\n",thread->getName());
   return thread;
/*#ifdef USER_PROGRAM  
   if(thread==NULL)
   {
     Stimulate();
     return (Thread *)readyList[i]->Remove();
   }
   return thread ;
#else
    return thread;
#endif */
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    //printf("next thread to run :%s\n",nextThread->getName());
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    // machine->pageTable=currentThread->space->pageTable; 
    SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
	        
	delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
     #ifdef USER_PROGRAM
     if (currentThread->space != NULL) {     // if there is an address space
        
        currentThread->RestoreUserState();     // to restore, do it.
    currentThread->space->RestoreState();}
    #endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList[0]->Mapcar((VoidFunctionPtr) ThreadPrint);
}
#ifdef USER_PROGRAM
void 
Scheduler::SuspendReady(Thread* thread)
{
    thread->status= ReadySuspend;
    suspendList->Append(thread);
    for(int i=0;i<thread->space->numPages;i++)
    {
        if(thread->space->pageTable[i].valid)
        {
            thread->space->pageTable[i].valid=FALSE;
            int ppn=thread->space->pageTable[i].physicalPage;
            int vpn=thread->space->pageTable[i].virtualPage;
            machine->inverseTable[ppn].owner=NULL;
            MemoryMap->Clear(ppn);
            memcpy(thread->space->swapArea+vpn*PageSize,
                machine->mainMemory+ppn*PageSize,
            PageSize);
        }
    }
    printf("make %s from Ready to Suspend\n",thread->getName());
}
void
Scheduler::SuspendSleep(Thread* thread)
{
    thread->status = SleepSuspend;
    sleepsusList->Append(thread);
    for(int i=0;i<thread->space->numPages;i++)
    {
        if(thread->space->pageTable[i].valid)
        {
            thread->space->pageTable[i].valid=FALSE;
            int ppn=thread->space->pageTable[i].physicalPage;
            int vpn=thread->space->pageTable[i].virtualPage;
            machine->inverseTable[ppn].owner=NULL;
            MemoryMap->Clear(ppn);
            memcpy(thread->space->swapArea+vpn*PageSize,
                machine->mainMemory+ppn*PageSize,
            PageSize);
        }
    }
    printf("make %s from sleep to suspend\n",thread->getName());
}

void 
Scheduler::Stimulate()
{
    Thread* thread=(Thread*)(suspendList->Remove());
    if(thread!=NULL)
    {
        ReadyToRun(thread);
        printf("stimulate %s from Suspend to Ready\n",thread->getName());
        return;
    }
    else
    {
        thread=(Thread*)(sleepsusList->Remove());
        if(thread!=NULL)
        {
            printf("stimulate %s from Suspend to Sleep\n",thread->getName());
            thread->status= BLOCKED;
        }
    }
     
}
#endif