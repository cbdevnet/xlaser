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

	if(config->dmx_address == 0){
		fprintf(stderr, "Invalid address provided, must be between 1 and 512\n");
		return 1;
	}

	if (config->dmx_address + DMX_CHANNELS > 512) {
		fprintf(stderr, "DMX Adress is too high for this fixture (%d + %d > 512).\n", config->dmx_address, DMX_CHANNELS);
		return 1;
	}

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

int config_remap(const char* category, char* key, char* value, EConfig* econfig, CONFIG* config){
	unsigned u;
	char* token = NULL;
	for(u = 0; u < DMX_CHANNELS; u++){
		if(value && CHANNEL_NAME[u] && !strcmp(CHANNEL_NAME[u], key)){
			//FIXME can econfig handle this?
			token = strtok(value, " ");
			do{
				if(!strcmp(token, "fixed")){
					config->dmx_config[u].fixed = true;
					token = strtok(NULL, " ");
					if(token){
						config->dmx_config[u].min = strtoul(token, NULL, 0);
					}
					else{
						fprintf(stderr, "Parameter expected for keyword\n");
						return -1;
					}
				}
				else if(!strcmp(token, "source")){
					token = strtok(NULL, " ");
					if(token){
						config->dmx_config[u].source = strtoul(token, NULL, 0);
					}
					else{
						fprintf(stderr, "Parameter expected for keyword\n");
						return -1;
					}
				}
				else if(!strcmp(token, "min")){
					token = strtok(NULL, " ");
					if(token){
						config->dmx_config[u].min = strtoul(token, NULL, 0);
					}
					else{
						fprintf(stderr, "Parameter expected for keyword\n");
						return -1;
					}
				}
				else if(!strcmp(token, "max")){
					token = strtok(NULL, " ");
					if(token){
						config->dmx_config[u].max = strtoul(token, NULL, 0);
					}
					else{
						fprintf(stderr, "Parameter expected for keyword\n");
						return -1;
					}
				}
				else if(!strcmp(token, "inverted")){
					config->dmx_config[u].inverted = true;
				}

				token = strtok(NULL, " ");
			}
			while(token);

			fprintf(stderr, "Remapped channel %s\n", key);
			return 0;
		}
	}
	fprintf(stderr, "No channel with name %s found to remap\n", key);
	return -1;
}

int arg_address(int argc, char** argv, CONFIG* config){
	config->dmx_address = strtoul(argv[1], NULL, 10);
	if (config->dmx_address == 0) {
		return -1;
	}
	if (config->dmx_address + DMX_CHANNELS > 512) {
		fprintf(stderr, "DMX Adress is too high for this fixture (%d + %d > 512).\n", config->dmx_address, DMX_CHANNELS);
		return -1;
	}
	return 0;
}

int arg_help(int argc, char** argv, CONFIG* config){
	exit(usage("xlaser"));
	return 0;
}

int parse_config(CONFIG* config, char* filepath){
	unsigned u;
	EConfig* econfig = econfig_init(filepath, config);

	unsigned artNetCat = econfig_addCategory(econfig, "artnet");
	unsigned dmxCat = econfig_addCategory(econfig, "dmx");
	unsigned genCat = econfig_addCategory(econfig, "general");
	unsigned windowCat = econfig_addCategory(econfig, "window");
	unsigned remapCat = econfig_addCategory(econfig, "remap");

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

	for(u = 0; u < DMX_CHANNELS; u++){
		if(CHANNEL_NAME[u]){
			econfig_addParam(econfig, remapCat, CHANNEL_NAME[u], config_remap);
		}
	}

	econfig_parse(econfig);
	econfig_free(econfig);
	return 0;
}

int parse_args(CONFIG* config, int argc, char** argv, char** output){
	eargs_addArgument("-d", "--dmx", arg_address, 1);
	eargs_addArgument("-h", "--help", arg_help, 0);
	return eargs_parse(argc, argv, output, config);
}


