#include "def.h"

int get_commandline(char *str, char *av[])
{
	int ac = 0;
	char delimiter[] = " \t\n";
	
	av[ac] = strtok(str, delimiter);
	while(av[ac] != NULL){
		ac++;
		av[ac] = strtok(NULL, delimiter);
	}
	if(av[0] == NULL){
		return -1;
	}else{
		return ac;
	}
}
