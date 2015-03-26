#ifndef STR_SPLIT_H

#include <vector>
#include <string>

typedef enum {
    STR_FLAG_NONE       = 0,
    STR_FLAG_NO_EMPTY   = ( 1 << 0)     // don't add empty strings
} str_split_flag_t;                                                            

int str_split(std::vector<std::string> &dst, const char *src, const char *splt_ch,
	      str_split_flag_t flags = STR_FLAG_NONE);


#endif
