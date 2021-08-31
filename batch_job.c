/*This program is launched by  the dispatcher module */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <time.h>


int main(int argc, char * argv[]){
	time_t start,end;
	double diff = 0;
	time(&start);
	while(diff<atof(argv[1])){
		time(&end);
		diff = difftime(end,start);

	}
  exit(0);
}



