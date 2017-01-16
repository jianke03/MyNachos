// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "machine.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
bool Memory_LRU(int vpn)
{
	TranslationEntry *entry;
	int emptyid=MemoryMap->Find();
	if (emptyid>=0)// find a physical page
	{
		machine->pageTable[vpn].physicalPage=emptyid;
		machine->pageTable[vpn].valid=TRUE;
		machine->inverseTable[emptyid].owner=currentThread;
		machine->inverseTable[emptyid].vpn=vpn;
		entry=&(machine->pageTable[vpn]);
		for(int i=0;i<TLBSize;i++)
		{
			if(machine->tlb[i].virtualPage==vpn)
			{
				machine->tlb[i]=*(entry); // update the tlb table
				break;
			}
		}
		memcpy(machine->mainMemory+emptyid*PageSize,currentThread->
			space->swapArea+vpn*PageSize,PageSize);
		// load the page from swap area to mainMemory
		//printf("load vpn %d from swap area for thread %s\n",vpn,currentThread->getName());
		return TRUE; 
	}
	else// can not find a empty physical page
	{
		int maxvalue=-1;
		int replaceid=-1;//  physical page to be replace
		for(int i=0;i<NumPhysPages;i++)
		{
			int tempvalue=machine->inverseTable[i].phy_lastuse;
			if(tempvalue>maxvalue)
			{
				maxvalue=tempvalue;
				replaceid=i;
			}
		}
		Thread* replaceThread=(Thread*)(machine->inverseTable[replaceid].owner);
		int replacevpn = machine->inverseTable[replaceid].vpn;
		replaceThread->space->pageTable[replacevpn].valid=FALSE;
		memcpy(replaceThread->space->swapArea+replacevpn*PageSize,machine->mainMemory+replaceid*PageSize,
			PageSize);
		if(replaceThread==currentThread)
		{
			for(int j=0;j<TLBSize;j++)
			{
				if(machine->tlb[j].virtualPage==replacevpn)
					machine->tlb[j].valid=FALSE;
			}
		}
		machine->pageTable[vpn].physicalPage=replaceid;
				machine->pageTable[vpn].valid=TRUE;
		machine->inverseTable[replaceid].owner=(void*)(currentThread);
		machine->inverseTable[replaceid].vpn=vpn;
		entry=&(machine->pageTable[vpn]);
		for(int i=0;i<TLBSize;i++)
		{
			if(machine->tlb[i].virtualPage==vpn)
			{
				machine->tlb[i]=*(entry); // update the tlb table
				break;
			}
		}
		memcpy(machine->mainMemory+replaceid*PageSize,
			currentThread->space->swapArea+vpn*PageSize,PageSize);
		//printf("load vpn %d from swap area for thread %s\n",vpn,currentThread->getName());
		return TRUE; 
	}
}
bool  TLB_LRU(int vpn)
{
   TranslationEntry *entry;
   //if(currentThread->getName()[1]=='3')
   //{
   //	printf("want to find vpn: %d \n",vpn,machine->pageTableSize);
   //}
   if (vpn >= machine->pageTableSize)
		{
			printf("vpn >= pageTableSize\n");		   
		    return FALSE;
		}
		entry = &(machine->pageTable[vpn]);

	/*for(int k=0;k<currentThread->space->numPages;k++)
    {

    	
    	printf("cur k: %d vpn: %d ppn: %d\n",k,currentThread->space->pageTable[k].virtualPage,currentThread->space->pageTable[k].physicalPage);	
	}
	for(int k=0;k<currentThread->space->numPages;k++)
    {

    	printf("machine k: %d vpn: %d ppn: %d\n",k,machine->pageTable[k].virtualPage,machine->pageTable[k].physicalPage);
    	
	}*/
	//printf("%d %d %d\n",vpn,entry->virtualPage,entry->physicalPage);	
	int TLBIndex=-1;
	for(int i=0;i<TLBSize;i++)
	{
		if(!machine->tlb[i].valid)
		{
			TLBIndex=i;
			break;
		}
	}
	if(TLBIndex!=-1)//find an empty space for new entry;
	{
		for(int i=0;i<TLBSize;i++)
		{
			if(machine->tlb[i].valid)
			{
				machine->TLB_lastuse[i]+=1;
			}
		}
		machine->tlb[TLBIndex]=*entry;
		machine->tlb[TLBIndex].valid=TRUE;
		machine->TLB_lastuse[TLBIndex]=0;
	}
	else
	{
		int LRUIndex=-1;
		int LRUValue=-1;
		for(int i = 0 ;i < TLBSize;i++)
		{
			if(machine->TLB_lastuse[i]>LRUValue)
				{
					LRUIndex=i;
					LRUValue=machine->TLB_lastuse[i];
				}
		}
		for(int i=0;i<TLBSize;i++)
		{
			machine->TLB_lastuse[i]+=1;
		}
		machine->tlb[LRUIndex]=*entry;
		machine->tlb[LRUIndex].valid=TRUE;
		machine->TLB_lastuse[LRUIndex]=0;
	}
	if (!machine->pageTable[vpn].valid)
	{
		Memory_LRU(vpn);
		return TRUE;
	}		
	return TRUE;
}		
bool TLB_ROUND(int vpn)
{
	
   TranslationEntry *entry;
   if (vpn >= machine->pageTableSize)
		{
			printf("vpn >= pageTableSize\n");		   
		    return FALSE;
		}
		else if (!(machine->pageTable[vpn].valid)) 
		{
		    printf("page is not valid");
		    return FALSE;
		}
		entry = &(machine->pageTable[vpn]);
	int TLBIndex=-1;
	for(int i=0;i<TLBSize;i++)
	{
		if(!machine->tlb[i].valid)
		{
			TLBIndex=i;
			break;
		}
	}
	if(TLBIndex!=-1)//find an empty space for new entry;
	{
		machine->tlb[TLBIndex]=*entry;
		machine->tlb[TLBIndex].valid=TRUE;
	}
	else
	{
		machine->tlb[machine->replaceIndex]=*entry;
		machine->tlb[machine->replaceIndex].valid=TRUE;
		machine->replaceIndex=(machine->replaceIndex+1)%TLBSize;
	}
	return TRUE;		
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    switch(which)
    {
    	case SyscallException:
    	{
	    	switch(type)
	    	{
	    		case SC_Halt:
	    		{
	    			DEBUG('a', "Shutdown, initiated by user program.\n");
	   				interrupt->Halt();
	   				break;
	   			}
	   			case SC_Exit:
	   			{

	   	   			DEBUG('a',"user program exit\n");
	   				int exitstatus=machine->ReadRegister(4);
	   				printf("USER EXIT STATUS:%d\n",exitstatus);
	    			//Exit(exitstatus);
	    			printf("%s finish \n",currentThread->getName());
	    			delete currentThread->space;
	    			currentThread->Finish();
	    			break;
	    		}
	    		default: 
	    		{
	    			printf("Unexpected SyscallException\n");	
	    			ASSERT(FALSE);
	    			break;
	    		}			
	    	}
	    	break;
	    }
	    case  PageFaultException:
	    {
	    	int badaddr=machine->registers[BadVAddrReg];
	    	int vpn = (unsigned) badaddr / PageSize;
	    	if(TLB_LRU(vpn))
	    	{
	    		break;
	    	}
	    	else
	    	{
	    		ASSERT(FALSE);
	    	}
	    	break;
	    }
	    default:
	    {
	    	printf("Unexpected user mode exception %d %d\n", which, type);
	    	ASSERT(FALSE);
	    	break;
	    }	
    }
}
