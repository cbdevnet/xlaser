#pragma once

typedef struct ECParam ECParam;
struct ECParam {
	const char* name;
	void* f;
	ECParam* next;	
};


typedef struct ECCategory ECCategory;
struct ECCategory {
	const char* name;
	unsigned id;
	ECParam* params;
	ECCategory* next;
};


typedef struct EConfig {
	const char* file;
	char* delim;
	unsigned lastid;
	ECCategory* categories;
	void* user_param;
} EConfig;

typedef enum {
	EC_SUCCESS = 0,
	EC_CANNOT_OPEN_FILE = -1,
	EC_CAT_NOT_FOUND = -2,
	EC_PARSING_ERROR = -3,
	EC_KEY_NOT_FOUND = -4
} EC_ERRORS;


EConfig* econfig_init(const char* file, void* user_param);
unsigned econfig_addCategory(EConfig* config, const char* category);
int econfig_addParam(EConfig* config, unsigned category, const char* param, void* f);
int econfig_parse(EConfig* config);
void econfig_free(EConfig* config);
