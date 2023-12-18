
#ifndef _options_h_
#define _options_h_

typedef struct params {
    int width;
    int height;
    int left;
    int top;
    int transform;
    int blackground;
	float scale;
} params_t;

int parse_options(int argc, char** argv, params_t* p);

#endif
