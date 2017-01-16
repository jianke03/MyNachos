// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "synch.h" 
// testnum is set in main.cc
int testnum = 1;
/*Semaphore* full;
Semaphore* empty;
Semaphore* mutex;
int count;
Lock* mylock;
Condition* fullsig;
Condition* emptysig;
int fullNum;
int emptyNum;*/
/*class RWLock
{
public:
	RWLock(char* debugName)
	{
		name=debugName;
		semaphore=new Semaphore(debugName,1);
		ownerThread=NULL;
		sharemode=0;
	}
	~RWLock()
	{
		delete semaphore;
	}
	void Acquire(int mode)
	{
		semaphore->P();
		sharemode=mode;
   		ownerThread=currentThread;
	}
	void Release(int mode)
	{
		ASSERT(isHeldByCurrentThread(mode));
  		ownerThread=NULL;
  		semaphore->V();
	}
	bool isHeldByCurrentThread(int mode)
	{
		if(sharemode==1)
		{
			return sharemode==mode;
		}
		else
			return ownerThread==currentThread;
	}
private:
     	char* name;				
    	Semaphore* semaphore;
    	Thread* ownerThread;
    	int sharemode;	
};
Lock* mutex;
RWLock* writelock;
int file;
int readcount;
void write(int n)
{
  for(int i=0;i<5;i++)
  {
     writelock->Acquire(0);
     file=n*10+i;
     printf("%s is writing file as %d\n",currentThread->getName(),n*10+i);
     interrupt->OneTick();
     writelock->Release(0);
  }
}
void read(int n)
{
   for(int i=0;i<n;i++)
   {
     mutex->Acquire();
     readcount++;
     if (readcount==1)
       writelock->Acquire(1);
     mutex->Release();
     printf("%s is reading file as %d\n",currentThread->getName(),file);
     interrupt->OneTick();
     mutex->Acquire();
     readcount--;
     if (readcount==0)
      writelock->Release(1);
     mutex->Release();
   }
  
}
void ThreadTest1()
{
   file=0;
   readcount=0;
   mutex=new Lock("mutexforrc");
   writelock=new RWLock("writelock");
   Thread* r1=Thread::CreateThread("r1");
   Thread* w1=Thread::CreateThread("w1");
   Thread* r2=Thread::CreateThread("r2");
   Thread* w2=Thread::CreateThread("w2");
   Thread* r3=Thread::CreateThread("r3");
   w1->Fork(write,1);
   r1->Fork(read,2);   
   r2->Fork(read,2);
   w2->Fork(write,2);
   r3->Fork(read,2);
}*/
class Barrier
{
   public:
      Barrier(int n)
      {
         my_lock=new Lock("my_lock");
         goon=new Condition("goon");
         partNum=n;
         reachNum=0;
      }
     ~Barrier()
     {
        delete my_lock;
        delete goon;
     }
     void ReachBarrier()
     {
        my_lock->Acquire();
        reachNum++;
        if (reachNum<partNum)
          goon->Wait(my_lock);
        if (reachNum==partNum)
          goon->Broadcast(my_lock);
        my_lock->Release();
     }
   private:
          int reachNum;
          int partNum;
          Lock* my_lock;
          Condition* goon;
          
};
Barrier* myBarrier;
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times  %d\n", which, num,currentThread->priority);
        currentThread->Yield();
    }
}
//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------
//global variables for producer-consumer problem
/////////////////////////////////////////////////////////////////Barrier test code
void stepBarrier(int n)
{
   for(int i=0;i<n;i++)
   {
     printf("%s %d th step\n",currentThread->getName(),i+1); 
     interrupt->OneTick();
   }
   printf("%s reach Barrier \n",currentThread->getName());
   myBarrier->ReachBarrier();
   printf("%s can go on now\n",currentThread->getName());
   printf("%s %d th step\n",currentThread->getName(),n+1);
   interrupt->OneTick();   
}
void ThreadTest1()
{
   /*myBarrier=new Barrier(3);
   Thread* t1=Thread::CreateThread("t1");
   Thread* t2=Thread::CreateThread("t2");
   Thread* t3=Thread::CreateThread("t3");
   t1->Fork(stepBarrier,2);
   t2->Fork(stepBarrier,3);
   t3->Fork(stepBarrier,4);*/
}
////////////////////////////////////////////////////////////////////Semaphore test code for procon
/*void producer(int n)
{
   for(int i=0;i<n;i++)
   {
      empty->P();
      mutex->P();
      count++;
      printf("%s total product num is %d \n",currentThread->getName(),count);
      interrupt->OneTick();
     mutex->V();
     full->V();       
   }
}
void consumer(int n)
{
   for(int i=0;i<n;i++)
   {
      full->P();
      mutex->P();
      count--;
      printf("%s total product num is %d at \n",currentThread->getName(),count);
      interrupt->OneTick();
      mutex->V();
      empty->V();
   }
}
void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");
    full= new Semaphore("full",0);
    empty= new Semaphore("empty",6);
    mutex=new Semaphore("mutex",1);
    Thread* t1=Thread::CreateThread("t1");
    Thread* t2=Thread::CreateThread("t2");
    Thread* t3=Thread::CreateThread("t3");
    count=0;
    t1->Fork(producer,10);
    t2->Fork(consumer,5);
    t3->Fork(consumer,5);     
}
*/
//////////////////////////////////////////////condition test code for procon problem
/*
void producer(int n)
{
  for(int i=0;i<n;i++)
  {
     mylock->Acquire();
     if (emptyNum==0)
        emptysig->Wait(mylock);
     fullNum++;
     emptyNum--;
     count++;
     printf("%s total product num is %d emptyNum:%d  fullNum: %d \n",currentThread->getName(),count,emptyNum,fullNum);
     interrupt->OneTick();
     if (fullNum==1)
        fullsig->Signal(mylock);
     mylock->Release();
  }
}
void consumer(int n)
{
   for(int i=0;i<n;i++)
   {
      mylock->Acquire();
      if (fullNum==0)
      fullsig->Wait(mylock);
      emptyNum++;
      fullNum--;
      count--;
      printf("%s total product num: %d emptyNum: %d fullNum: %d \n",currentThread->getName(),count,emptyNum,fullNum);
      interrupt->OneTick();
      if (emptyNum==1)
	emptysig->Signal(mylock);
      mylock->Release();
   }
}
void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");
    mylock=new Lock("mylock");
    fullsig=new Condition("fullsig");
    emptysig=new Condition("emptysig");
    fullNum=0;
    emptyNum=6;
    Thread* t1=Thread::CreateThread("t1");
    Thread* t2=Thread::CreateThread("t2");
    Thread* t3=Thread::CreateThread("t3");
    count=0;
    t1->Fork(producer,10);
    t2->Fork(consumer,5);
    t3->Fork(consumer,5);     
}*/ 
void simulatetime(int n)
{
   for(int i=0;i<n;i++)
   interrupt->OneTick();
}
void ThreadTest2()
{
  Thread* t1=Thread::CreateThread("t1"); 
  Thread* t2=Thread::CreateThread("t2"); 
  Thread* t3=Thread::CreateThread("t3");  
  t1->Fork(simulatetime,20);
  t2->Fork(simulatetime,30);
  t3->Fork(simulatetime,40);	
}
void ThreadTest3()
{
  Thread* t; 
  for(int i=0;i<130;i++)
{
   t=Thread::CreateThread("GAO");
    if (t==NULL)
   printf("it overcount 128\n");
   else
   ThreadPrint(t);
}
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
        ThreadTest2(); 
        break;
    case 3:
        ThreadTest3();
        break;  
    default:
	printf("No test specified.\n");
	break;
    }
}

