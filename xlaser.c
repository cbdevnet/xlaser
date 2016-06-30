#include "xlaser.h"

int usage(char* fn){
	printf("xlaser - ArtNet scanner fixture\n");
	printf("Usage:\n");
	printf("\t%s <path to config file> [-d <addr> | --dmx <addr> ]\n", fn);
	return EXIT_FAILURE;
}

int config_artSubUni(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;
	config->art_subUni = strtoul(value, NULL, 10);

	return 0;
}

int config_artUni(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;
	config->art_universe = strtoul(value, NULL, 10);

	return 0;
}

int config_dmxAddress(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {
	CONFIG* config = (CONFIG*) user_param;

	// if the address is set by commandline argument.
	if (config->dmx_address > 0) {
		return 0;
	}

	config->dmx_address = strtoul(value, NULL, 10);

	return 0;
}

int config_artNet(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;

	config->art_net = strtoul(value, NULL, 10);

	return 0;
}

int config_bindhost(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;

	config->bindhost = malloc(strlen(value) + 1);
	strncpy(config->bindhost, value, strlen(value) + 1);

	return 0;
}

int config_windowed(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;

	int b = econfig_getBoolean(value);

	printf("windowed?: %d\n", b);

	if (b < 0) {
		return 1;
	}

	config->windowed = (bool) b;

	return 0;
}

int config_width(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;
	config->window_width = strtoul(value, NULL, 10);

	return 0;
}

int config_height(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;
	config->window_height = strtoul(value, NULL, 10);

	return 0;
}

int config_x_offset(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;
	config->x_offset = strtoul(value, NULL, 10);

	return 0;
}

int config_y_offset(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;
	config->y_offset = strtoul(value, NULL, 10);

	return 0;
}

int config_gobo_folder(const char* category, char* key, char* value, EConfig* econfig, void* user_param) {

	CONFIG* config = (CONFIG*) user_param;
	unsigned len = strlen(value);

	config->gobo_prefix = malloc(len + 2);
	strncpy(config->gobo_prefix, value, len + 1);

	// add trailing slash if not found.
	if (value[len - 1] != '/') {
		config->gobo_prefix[len] = '/';
	}
	config->gobo_prefix[len + 1] = 0;
	return 0;
}
int parse_config(CONFIG* config, char* filepath) {

	EConfig* econfig = econfig_init(filepath, config);

	unsigned artNetCat = econfig_addCategory(econfig, "artnet");
	unsigned dmxCat = econfig_addCategory(econfig, "dmx");
	unsigned genCat = econfig_addCategory(econfig, "general");
	unsigned windowCat = econfig_addCategory(econfig, "window");

	econfig_addParam(econfig, artNetCat, "net", config_artNet);
	econfig_addParam(econfig, artNetCat, "subuni", config_artSubUni);
	econfig_addParam(econfig, artNetCat, "uni", config_artUni);


	econfig_addParam(econfig, dmxCat, "address", config_dmxAddress);

	econfig_addParam(econfig, genCat, "bindhost", config_bindhost);
	econfig_addParam(econfig, genCat, "gobos", config_gobo_folder);

	econfig_addParam(econfig, windowCat, "windowed", config_windowed);
	econfig_addParam(econfig, windowCat, "width", config_width);
	econfig_addParam(econfig, windowCat, "height", config_height);
	econfig_addParam(econfig, windowCat, "x_offset", config_x_offset);
	econfig_addParam(econfig, windowCat, "y_offset", config_y_offset);

	econfig_parse(econfig);
	econfig_free(econfig);

	return 0;
}


int setDMXAddress(int argc, char** argv, CONFIG* config) {
	config->dmx_address = strtoul(argv[1], NULL, 10);

	if (config->dmx_address == 0) {
		return -1;
	}

	return 0;
}

int getHelp() {
	exit(usage("xlaser"));

	return 0;
}

int parse_args(CONFIG* config, int argc, char** argv, char** output) {

	eargs_addArgument("-d", "--dmx", setDMXAddress, 1);
	eargs_addArgument("-h", "--help", getHelp, 0);

	return eargs_parse(argc, argv, output, config);
}

int main(int argc, char** argv){
	CONFIG config = {
		.dmx_address = 0,
		.windowed = false,
		.window_name = "xlaser",
		.bindhost = strdup("*"),
		.gobo_prefix = strdup("gobos/"),
		.gobo = {}
	};

	XRESOURCES xres = {};

	char* invalid_arguments[argc];
	int invalid_arguments_len = parse_args(&config, argc, argv, invalid_arguments);

	if (invalid_arguments_len < 0) {
		fprintf(stderr, "Error parsing commandline arguments\n");
		exit(usage(argv[0]));
	} else if(invalid_arguments_len < 1){
		fprintf(stderr, "Need at least a configuration file\n");
		exit(usage(argv[0]));
	}

	parse_config(&config, output[0]);

	if (config.dmx_address == 0) {
		config.dmx_address = 1;
	}

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

	//TODO cleanup
	free(config.bindhost);
	free(config.gobo_prefix);

	return 0;
}
