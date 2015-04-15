#include <algorithm>
#include <vector>
#include <string>
#include <stdlib.h>
#include <stdio.h>
extern "C" {
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
};
#include <linenoise.h>
#include "spawn.h"
#include "str_split.h"
#include "lua-shell.h"

using namespace std;

static const char *
find_table_start (const char *table_path, string &leading)
{
    const char *end_p;

    end_p = table_path + strlen(table_path) - 1;

    for (; end_p > table_path; end_p--) {
	int ch = *end_p;

	if (isalnum(ch))
	    continue;
	if (ch == '.' || ch == '_')
	    continue;

	break;
    }

    if (end_p > table_path) {
	string sb;
	sb = table_path;

	leading = sb.substr(0, end_p - table_path + 1);
	return end_p + 1;
    }

    return table_path;
}

/*
 * Given a 'table_path' of form "edo.util.foo.bar",
 * pushes the final table on to the Lua stack,
 * in the example, 'foo' is pushed to the stack
 *
 * the leading string arg is set to "edo.util.foo"
 * and the last token "bar" is set to trailing.
 *
 * The top of lua stack contains the result
 * of the push, either nil, or the table.
 */
static void
push_lua_table (lua_State *L, const char *table_path,
		string &leading, string &trailing)
{
    vector<string> toks;
    string str;

    /*
     * table_path may contain more than the table, like
     *      my_function(edo.table.foo<tab>
     * Here we want to start looking for a table starting
     * from the 'e' in 'edo.table'
     */
    table_path = find_table_start(table_path, leading);

    str_split(toks, table_path, ".");

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    for (unsigned i = 0; i < (toks.size() - 1); i++) {
	lua_getfield(L, -1, toks[i].c_str());

	/*
	 * We got a table from the field, or nil, in either case the
	 * original table isn't need so is removed from the stack
	 */
	lua_remove(L, -2);

	if (lua_isnil(L, -1)) {
	    return;
	}

	leading += toks[i].c_str();
	leading += ".";
    }
    if (toks.size() > 0)
	trailing = toks[toks.size() - 1];
}

/*
 * compares two strings, 'leading' against 'compare'
 * returns non-zero if 'leading' is a substring of 'compare'
 * or if leading is NULL or an empty string
 */
static inline int
leading_cmp (const char *leading, const char *compare)
{
    if (compare == NULL)
        return 0;

    if (leading == NULL || *leading == 0)
        return 1;

    return *leading == *compare
		&& strncmp(leading, compare, strlen(leading)) == 0;
}

void
lshell_completion (const char *buf, linenoiseCompletions *lc)
{
    lua_State *L = lshell_get_L();
    int old_top = lua_gettop(L);
    vector<string> matched_toks;

    string leading, trailing;

    push_lua_table(L, buf, leading, trailing);
    if (lua_isnil(L, -1))
	goto cleanup;

    /*
     * Iteration over a table setup. Stack has
     *   [table][key = nil]
     * on each iteration key moves forward, lua_next()
     * pops the key and pushes a new key, value to the stac
     *   [table][key = 'foo'][value = xx]
     * A copy of the key is made before using as some routines
     * may modify the key in place.
     *
     * lua_next() doesn't push anything to stack when it is at
     * end of table.
     */
    lua_pushvalue(L, -1);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
	lua_pushvalue(L, -2);

	const char *key = lua_tostring(L, -1);

	if (leading_cmp(trailing.c_str(), key)) {
	    string s = leading + key;
	    matched_toks.push_back(s);
	}
	lua_pop(L, 2);
    }
    lua_pop(L, 1);

    sort(matched_toks.begin(), matched_toks.end());
    for (auto s : matched_toks) {
	linenoiseAddCompletion(lc, s.c_str(), "");
    }
 cleanup:
    lua_settop(L, old_top);
}
