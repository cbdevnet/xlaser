int config_artSubUni(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	config->art_subUni = strtoul(value, NULL, 10);
	return 0;
}

int config_dmxAddress(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	//do not override if set by commandline argument
	if(config->dmx_address > 0){
		return 0;
	}
	config->dmx_address = strtoul(value, NULL, 10);
	return 0;
}

int config_artNet(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	config->art_net = strtoul(value, NULL, 10);
	return 0;
}

int config_bindhost(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	if(config->bindhost){
		free(config->bindhost);
	}
	config->bindhost = strdup(value);
	return 0;
}

int config_windowed(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	int b = econfig_getBoolean(value);
	if(b < 0){
		return 1;
	}
	config->windowed = (bool)b;
	return 0;
}

int config_width(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	config->window_width = strtoul(value, NULL, 10);
	return 0;
}

int config_height(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	config->window_height = strtoul(value, NULL, 10);
	return 0;
}

int config_x_offset(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	config->x_offset = strtoul(value, NULL, 10);
	return 0;
}

int config_y_offset(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	config->y_offset = strtoul(value, NULL, 10);
	return 0;
}

int config_gobo_folder(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	if(config->gobo_prefix){
		free(config->gobo_prefix);
	}
	config->gobo_prefix = strdup(value);
	return 0;
}

int arg_address(int argc, char** argv, CONFIG* config){
	config->dmx_address = strtoul(argv[1], NULL, 10);
	if (config->dmx_address == 0) {
		return -1;
	}
	return 0;
}

int arg_help(int argc, char** argv, CONFIG* config){
	exit(usage("xlaser"));
	return 0;
}

int parse_config(CONFIG* config, char* filepath){
	EConfig* econfig = econfig_init(filepath, config);

	unsigned artNetCat = econfig_addCategory(econfig, "artnet");
	unsigned dmxCat = econfig_addCategory(econfig, "dmx");
	unsigned genCat = econfig_addCategory(econfig, "general");
	unsigned windowCat = econfig_addCategory(econfig, "window");

	econfig_addParam(econfig, artNetCat, "net", config_artNet);
	econfig_addParam(econfig, artNetCat, "subuni", config_artSubUni);

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

int parse_args(CONFIG* config, int argc, char** argv, char** output){
	eargs_addArgument("-d", "--dmx", arg_address, 1);
	eargs_addArgument("-h", "--help", arg_help, 0);
	return eargs_parse(argc, argv, output, config);
}


