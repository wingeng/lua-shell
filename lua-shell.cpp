#include <vector>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
extern "C" {
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
};
#include <termios.h>
#include <linenoise.h>
#include "lua-shell.h"
#include "spawn.h"

using namespace std;

class interp_c {
public:
    interp_c(const char *prompt_1, const char *prompt_2);
    ~interp_c();

    void init_lua();

    void load_line(const char *line);
    void run_line();

    lua_State *L() {return i_lua_state;}

    const char *prompt();

public:
    string i_prompt_cur;
    string i_p1, i_p2;
    lua_State *i_lua_state;

private:
    string i_line_buffer;
};

/*
 * Global interpreter since linenoise doesn't have
 * callback data
 */
interp_c lua("lua> ", "    >> ");

interp_c::
interp_c (const char *prompt_1, const char *prompt_2) :
    i_p1(prompt_1), i_p2(prompt_2)
{
    i_prompt_cur = i_p1;
}

interp_c::
~interp_c ()
{
    if (i_lua_state) {
	lua_close(i_lua_state);
	i_lua_state = NULL;
    }
}

void interp_c::
init_lua (void)
{
    i_lua_state = luaL_newstate();  
    luaL_openlibs(i_lua_state);
}

const char *interp_c::
prompt ()
{
    return i_prompt_cur.c_str();
}

void interp_c::
load_line (const char *line)
{
    i_line_buffer = i_line_buffer + "\n" + line;
}

void interp_c::
run_line ()
{
    string lbuf, last_err;
    int error;

    lbuf = i_line_buffer;
    
    if (lbuf.size() > 0) {
	error = luaL_loadbuffer(L(), lbuf.c_str(), lbuf.size(), "line");
	if (error == 0)
	    error = lua_pcall(L(), 0, 0, 0);

	if (error) {
	    last_err = lua_tostring(L(), -1);
	    lua_pop(L(), 1);
	}

	/*
	 * Check if error was just incomplete syntax, ie dangling
	 * for loops, middle of writing a function, etc.
	 */
	if (error == LUA_ERRSYNTAX) {
	    auto found = last_err.find("<eof>");
	    if (found != std::string::npos) {
		i_prompt_cur = i_p2;
		return;
	    }
	}

	if (error) {
	    printf("%s", last_err.c_str());
	}

	i_line_buffer = "";
	i_prompt_cur = i_p1;
    }
}
    
void
completion (const char *buf, linenoiseCompletions *lc)
{
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc,"hello", "help for hello");
        linenoiseAddCompletion(lc,"hello there", "help for hello there");
    }
}

void
lshell_call (void *data)
{
    interp_c *lua = (interp_c *) data;

    lua->run_line();
}

lua_State *
lshell_get_L ()
{
    return lua.L();
}

int
main (int argc, char **argv)
{
    vector<int> process_ids;
    int write_fd;
    char *pipe_cmds;
    char *line;

    lua.init_lua();

    /* 
     * Set the completion callback. This will be called every time the user uses
     * the <tab> key.
     */
    linenoiseSetCompletionCallback(lshell_completion);

    /*
     * Load history from file. The history file is just a plain text file where
     * entries are separated by newlines.
     */
    linenoiseHistoryLoad("./lua_history"); /* Load the history at startup */

    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);

    while ((line = linenoise(lua.prompt())) != NULL) {
	linenoiseHistoryAdd(line); /* Add to the history. */
	linenoiseHistorySave("history.txt"); /* Save the history on disk. */

	if ((pipe_cmds = strchr(line, '|')) != NULL) {
	    *pipe_cmds = '\0';
	    pipe_cmds++;
	} else {
	    pipe_cmds = const_cast<char *>("less -X -E");
	}

	write_fd = make_pipe_line(pipe_cmds, process_ids);
	if (write_fd < 0)
	    break;

	lua.load_line(line);
	wrap_stdout(write_fd, lshell_call, &lua);

	wait_all_process(process_ids);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
	free(line);
    }

    printf("bye\n");

    return 0;
}
