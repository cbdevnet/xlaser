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

	CONFIG config = {
		.dmx_address = 16,
		.windowed = false,
		.window_name = "xlaser"
	};
	XRESOURCES xres = {};

	//TODO parse arguments & config file

	if(x11_init(&xres, &config) < 0){
		fprintf(stderr, "Failed to initialize window\n");
		exit(usage(argv[0]));
	}

	return 0;
}
