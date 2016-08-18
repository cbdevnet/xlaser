#include "xlaser.h"

int usage(char* fn){
	printf("%s - ArtNet scanner fixture\n", XLASER_VERSION);
	printf("Usage:\n");
	printf("\t%s <path to config file> [-d <addr> | --dmx <addr> ]\n", fn);
	return EXIT_FAILURE;
}

int getHelp() {
	exit(usage(SHORTNAME));

	return 0;
}

int main(int argc, char** argv){
	CONFIG config = {
		.dmx_address = 0,
		.windowed = false,
		.window_name = "xlaser",
		.bindhost = strdup("*"),
		.gobo_prefix = strdup("gobos/"),
	};

	XRESOURCES xres = {};
	printf("%s starting up\n", XLASER_VERSION);

	char* invalid_arguments[argc];
	int invalid_arguments_len = parse_args(&config, argc, argv, invalid_arguments);

	if (invalid_arguments_len < 0) {
		fprintf(stderr, "Error parsing commandline arguments\n");
		exit(usage(argv[0]));
	} else if(invalid_arguments_len < 1){
		fprintf(stderr, "Need at least a configuration file\n");
		exit(usage(argv[0]));
	}

	parse_config(&config, invalid_arguments[0]);

	//TODO sanity check config
	//TODO set up signal handlers
	if(x11_init(&xres, &config) < 0){
		fprintf(stderr, "Failed to initialize window\n");
		exit(usage(argv[0]));
	}

	//open artnet listener
	config.sockfd = udp_listener(config.bindhost, "6454");
	if(config.sockfd < 0){
		fprintf(stderr, "Failed to open ArtNet listener\n");
		exit(usage(argv[0]));
	}

	//run main loop
	xlaser(&xres, &config);

	//cleanup
	x11_cleanup(&xres);
	free(config.bindhost);
	free(config.gobo_prefix);

	return 0;
}
