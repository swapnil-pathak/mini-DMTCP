#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int main(int argc, char *argv[]){
	int i;
	for(i = 0; ; i++) {
		printf("%s ",".");
		fflush(stdout);
		sleep(1);
	}
}
