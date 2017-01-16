#include "copyright.h"
#include "synchconsole.h"
static void ConsoleReadDone(int arg)
{
	SynchConsole * synchconsole = (SynchConsole*)arg;
	synchconsole->ReadDone();
}
static void ConsoleWriteDone(int arg)
{
	SynchConsole * synchconsole = (SynchConsole*)arg;
	synchconsole->WriteDone();
}
SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
	Readsemaphore = new Semaphore("synch console read", 0);
	Writesemaphore = new Semaphore("synch console write",0);
    lock = new Lock("synch console lock");
    console = new Console(readFile, writeFile,ConsoleReadDone,
    	ConsoleWriteDone,(int)this);
}
SynchConsole::~SynchConsole()
{
	delete Readsemaphore;
	delete Writesemaphore;
	delete lock;
	delete console;
}
void 
SynchConsole::Wchar(char ch)
{
	lock->Acquire();
	console->PutChar(ch);
	Writesemaphore->P();
	lock->Release();
}
char 
SynchConsole::Rchar()
{
	char ch;
	lock->Acquire();
	Readsemaphore->P();
	ch= console->GetChar();
	lock->Release();
	return ch;
}
void 
SynchConsole::ReadDone()
{
	Readsemaphore->V();
}
void
SynchConsole::WriteDone()
{
	Writesemaphore->V();
}