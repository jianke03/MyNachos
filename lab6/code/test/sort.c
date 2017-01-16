/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"
int id=0;
char name[20]="Myhalt";
main()
{
     id = Exec(name);
     Join(id);
     Exit(0);
}    
