#include <string>
#include <vector>
#include <alloca.h>
#include <string.h>

#include "str_split.h"

using namespace std;

/*
 * Splits the string using 'split_chars'
 *
 * Returns the number of tokens and vector of str_buf
 */
int
str_split (vector<string> &dst, const char *src, const char *splt_ch,
	   str_split_flag_t flags)
{
    char *tok;
    char *buf;
    bool want_empty_str;

    buf = (char *) alloca(strlen(src) + 1);
    strcpy(buf, src);

    want_empty_str = (flags & STR_FLAG_NO_EMPTY) == 0;


    for (tok = strsep(&buf, splt_ch); tok; tok = strsep(&buf, splt_ch)) {
	if (tok) {
	    if (*tok || want_empty_str) {
		dst.push_back(tok);
	    }
	}
    }

    return dst.size();
}
