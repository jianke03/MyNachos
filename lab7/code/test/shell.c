#include "syscall.h"

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    int i;

    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
	Write(prompt, 2, output);

	i = 0;
	
	do {
	
	    Read(&buffer[i], 1, input); 

	} while( buffer[i++] != '\n' );

	buffer[--i] = '\0';

	if( i > 0 ) {
		if(buffer[0]=='e'&&buffer[1]==' ')
		{
			newProc = Exec(buffer+2);
			Join(newProc);
		}
		else if(buffer[0]=='h'&&buffer[1]=='e'&&
			buffer[2]=='l'&&buffer[3]=='p')
			{
				Help();
			}
		else if(buffer[0]=='p'&&buffer[1]=='w'&&buffer[2]=='d')
		{
			Pwd();
		}
		else if(buffer[0]=='l'&&buffer[1]=='s')
		{
			Ls();
		}
		else if(buffer[0]=='c'&&buffer[1]=='d'&&buffer[2]==' ')
		{
			Cd(buffer+3);
		}
		else if(buffer[0]=='m'&&buffer[1]=='f'&&buffer[2]==' ')
		{
			Create(buffer+3,200);
		}
		else if(buffer[0]=='m'&&buffer[1]=='d'&&buffer[2]==' ')
		{
			Md(buffer+3);
		}	
		else if(buffer[0]=='d'&&buffer[1]=='d'&&buffer[2]==' ')
		{
			Remove(buffer+3,1);
		}
		else if(buffer[0]=='d'&&buffer[1]=='f'&&buffer[2]==' ')
		{
			Remove(buffer+3,2);
		}
		else if(buffer[0]=='q')
		{
			Exit(0);
		}				
	}
    }
}

