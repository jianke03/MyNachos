/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"
int a[10];
int b[10];
int i=0;
main()
{
    for( i=0;i<10;i++)
    	a[i]=i+1;
    for( i=0;i<10;i++)
    	b[i]=a[i]*2;
    Exit(b[0]);
    //Halt();
    /* not reached */
}
