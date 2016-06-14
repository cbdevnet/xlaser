#include "xlaser.h"

int usage(char* fn){
	printf("xlaser - Whatever\n");
	printf("Usage:\n");
	printf("\t%s <dmx address> <bindhost>", fn);
	return EXIT_FAILURE;
}

int main(int argc, char** argv){
	if(argc < 3){
		exit(usage(argv[0]));
	}



	return 0;
}
