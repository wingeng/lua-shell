#include <ctype.h>
#include <string.h>

#include "parse_args.h"

using namespace std;

static bool
is_quote (char q)
{
    return q == '\'' || q == '"';
}

static const char *
skip_ws (const char *str)
{
    while (*str && isspace(*str))
	str++;

    return str;
}

static void
push_tok (vector<string> &toks, const char *s, int n)
{
    char buf[1024]; // replace with dynamic string ****
		    

    if (n > 0) {
	strncpy(buf, s, n);
	buf[n] = '\0';

	toks.push_back(buf);
    }
}

/*
 * Parse a string like 'grep  -v "foo bar"'
 * into the string vector.
 *
 * Returns the number of tokens found
 */
int
parse_args (const char *str, vector<string> &toks)
{
    bool in_quote = false;
    char quote_char = '\0';
    const char *seg_start;

    seg_start = skip_ws(str);

    for (; *str;  ) {
	if (in_quote) {
	    if (*str == quote_char) {
		in_quote = false;
		push_tok(toks, seg_start, str - seg_start);

		seg_start = str + 1;
	    } 

	    str++;
	    continue;
	}

	if (is_quote(*str)) {
	    in_quote = true;
	    quote_char = *str;

	    push_tok(toks, seg_start, str - seg_start);
	    seg_start = str + 1;

	    str++;
	    continue;
	}

	if (*str == '|') {
	    push_tok(toks, seg_start, str - seg_start);

	    toks.push_back("|");

	    str++;
	    str = skip_ws(str);
	    seg_start = str;

	    continue;
	}

	if (isspace(*str)) {
	    seg_start = skip_ws(seg_start);
	    push_tok(toks, seg_start, str - seg_start);

	    str = skip_ws(str);
	    seg_start = str;
	    continue;
	}

	str++;
    }

    push_tok(toks, seg_start, str - seg_start);

    return toks.size();
}

void
vsdump (vector<string> t)
{
    int i = 0;
    for (auto s : t) {
	printf("t[%d] '%s'\n", i, s.c_str());
	i++;
    }
}


#ifdef _TEST

#define TEST(b) \
    if (!(b)) { \
      fprintf(stderr, "%s:%d:Error: ", __FILE__, __LINE__); \
      fprintf(stderr, "'%s'\n:", #b); \
    } 


int
main (int argc, char **argv)
{
    vector<string>  toks;
    int n;

    toks.clear();
    n = parse_args("a|less", toks);
    //vsdump(toks);
    TEST(n == 3);
    TEST(toks[0] == "a");
    TEST(toks[1] == "|");
    TEST(toks[2] == "less");

    toks.clear();
    n = parse_args("a |  b    |    c  ", toks);
    // vsdump(toks);
    TEST(n == 5);
    TEST(toks[0] == "a");
    TEST(toks[1] == "|");
    TEST(toks[2] == "b");
    TEST(toks[3] == "|");
    TEST(toks[4] == "c");

    toks.clear();
    n = parse_args("a -d 'goo bye'", toks);
    
    TEST(n == 3);

    TEST(toks[0] == "a");
    TEST(toks[1] == "-d");
    TEST(toks[2] == "goo bye");


    toks.clear();
    n = parse_args("egrep '(99|89)' | less", toks);
    // vsdump(toks);

    TEST(n == 4);
    TEST(toks[0] == "egrep");
    TEST(toks[1] == "(99|89)");
    TEST(toks[2] == "|");
    TEST(toks[3] == "less");

    exit(0);
}

#endif
