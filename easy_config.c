#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "easy_config.h"


EConfig* econfig_init(const char* file, void* user_param) {
	EConfig* config = malloc(sizeof(EConfig));
	config->file = file;
	config->lastid = 0;
	config->categories = NULL;
	config->delim = "=";
	config->user_param = user_param;

	return config;
}

unsigned econfig_addCategory(EConfig* config, const char* category) {

	// malloc
	ECCategory* cat = malloc(sizeof(ECCategory));
	cat->name = category;
	cat->id = config->lastid;
	cat->params = NULL;
	cat->next = NULL;
	
	config->lastid++;
	
	if (config->categories) {
		ECCategory* c = config->categories;
		while (c->next) {
			c = c->next;
		}
		c->next = cat;
	} else {
		config->categories = cat;
	}

	return cat->id;
}


void econfig_createParam(ECCategory* cat, const char* param, void* f) {
	ECParam* eparam = malloc(sizeof(ECParam));
	eparam->name = param;
	eparam->f = f;
	eparam->next = NULL;

	if (cat->params) {
		ECParam* next = cat->params;
		while (next->next) {
			next = next->next;
		}
		next->next = eparam;
	} else {
		cat->params = eparam;
	}
}

int econfig_addParam(EConfig* config, unsigned category, const char* param, void* f) {

	ECCategory* next = config->categories;

	while(next) {
		if (next->id == category) {
			econfig_createParam(next, param, f);
			return 1;
		}
		next = next->next;
	}

	return 0;
}

void econfig_free_param(ECParam* param) {

	if (!param) {
		return;
	}

	econfig_free_param(param->next);
	free(param);
}

void econfig_free_category(ECCategory* cat) {

	if (!cat) {
		return;
	}

	econfig_free_category(cat->next);
	econfig_free_param(cat->params);
	free(cat);
}

void econfig_free(EConfig* config) {
	econfig_free_category(config->categories);
	free(config);
}

char* econfig_trimWhitespaces(char* line) {
	while (strlen(line) > 0 && isspace(line[0])) {
		line++;
	}
	unsigned i = strlen(line);
	while (i > 0 && isspace(line[i - 1])) {
		line[i - 1] = 0;
		i--;
	}

	return line;
}


int econfig_isComment(char* line) {
	
	// for us empty string is a comment
	if (strlen(line) < 1) {
		return 1;
	}
	
	switch (line[0]) {
		case '/':
			if (strlen(line) < 2 || line[1] != '/') {
				return 0;
			}
		case '#':
		case ';':
			return 1;
		default:
			return 0;
	}
}

int econfig_parseCategory(EConfig* config, char* line) {

	int len = strlen(line);

	// check empty category
	if (len < 2) {
		return EC_PARSING_ERROR;
	}

	// missing close bracket
	if (line[len - 1] != ']') {
		return EC_PARSING_ERROR;
	}

	line[len -1] = 0;
	line++;

	ECCategory* cat = config->categories;
	while (cat) {
		if (!strcmp(cat->name, line)) {
			return cat->id;
		}
		cat = cat->next;
	}

	return EC_CAT_NOT_FOUND;
}

ECCategory* econfig_getCategory(EConfig* config, unsigned category) {

	ECCategory* cat = config->categories;

	while (cat) {
		if (cat->id == category) {
			return cat;
		}
		cat = cat->next;
	}

	return NULL;
}

int econfig_parseParam(EConfig* config, char* key, char* value) {

	ECCategory* cat = econfig_getCategory(config, config->lastid);
	if (!cat) {
		return EC_PARSING_ERROR;
	}

	ECParam* param = cat->params;
	while (param) {
		if (!strcmp(param->name, key)) {
			int (*f)(const char* category, char* key, char* value, EConfig* econfig,  void* config) = param->f;
			return f(cat->name, key, value, config, config->user_param);
		}
		param = param->next;
	}

	return EC_KEY_NOT_FOUND;
}

int econfig_parseLine(EConfig* config, char* line, size_t len) {	
	// trim	
	line = econfig_trimWhitespaces(line);

	//comment
	if (econfig_isComment(line)) {
		return EC_SUCCESS;
	}
	
	// category
	if (line[0] == '[') {
		config->lastid = econfig_parseCategory(config, line);
		
		if (config->lastid < 0) {
			return config->lastid;
		} else {
			return EC_SUCCESS;
		}
	}
	len = strlen(line);
	// split
	char* key = strtok(line, config->delim);
	if (!key) {
		return EC_PARSING_ERROR;
	}
	if (len <= strlen(key) + 1) {
		return EC_PARSING_ERROR;
	}
	char* value = line + strlen(key) + 1;

	// trim splits
	key = econfig_trimWhitespaces(key);
	value = econfig_trimWhitespaces(value);

	// check key	
	return econfig_parseParam(config, key, value);
}

int econfig_parse(EConfig* config) {
	FILE* file;


	file = fopen(config->file, "r");

	if (!file) {
		return EC_CANNOT_OPEN_FILE;
	}

	char* line = NULL;
	size_t len = 0;
	int out = 0;
	ssize_t read = 0;
	while ((read = getline(&line, &len, file)) != -1) {
		if ((out = econfig_parseLine(config, line, len)) < 0) {
			free(line);
			return out;
		}
	}

	free(line);
	fclose(file);

	return EC_SUCCESS;
}
