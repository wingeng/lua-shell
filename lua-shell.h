#ifndef LUA_SHELL_H

void lshell_completion(const char *buf, linenoiseCompletions *lc);
lua_State *lshell_get_L();

#endif
