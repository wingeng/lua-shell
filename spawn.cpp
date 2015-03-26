/* 
 * Test of making of a pipe-line
 *
 * Copyright (c) 2015, Wing Eng
 * All rights reserved.
 */
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "spawn.h"
#include "parse_args.h"

using namespace std;

/*
 * Creates a child process string a pipe for
 * the child's stdin, returns the 'write' side of
 * the pipe for writing to.
 *
 *
 *  caller writes to pipe_in[1], child reads from pipe_in[0]
 *  caller reads from pipe_out[0], child writes to pipe_out[1] (if present)
 *
 * if pipe_out == NULL it means that the child should just write
 * to stdout.
 */
static int
add_child (vector<string> argv, int pipe_in[2], int pipe_out[2])
{
    int child_id;
    
    child_id = fork();

    if (child_id == 0) {
	dup2(pipe_in[0], STDIN_FILENO);

	close(pipe_in[0]);
	close(pipe_in[1]);

	if (pipe_out) {
	    dup2(pipe_out[1], STDOUT_FILENO);
	    close(pipe_out[0]);
	    close(pipe_out[1]);
	}

	/*
	 * copy over the argv to a writable string to make
	 * the compiler happy
	 */
	int i = 0;
	char * rargv[256];
	for (auto s : argv) {
	    rargv[i] = strdup(s.c_str());
	    i++;
	}
	rargv[i] = 0;

	
	if (execvp(rargv[0], rargv))
	    errx(-1, "error execing '%s'", rargv[0]);

    } else {

	/* 
	 * parent, doesn't read from the pipe-in
	 * and it doesn't write to pipe-out.
	 */
	close(pipe_in[0]);

	if (pipe_out) {
	    close(pipe_out[1]);
	}
	return child_id;
    }

    return 0;
}

/*
 * Make a pipe line of commands given an array of command sets
 * each element is a command with all it args
 *
 * Returns the file descriptor to write into, and the list of process ids
 *         that were forked/exec'ed to handle pipeline.
 */
int
make_pipe_line_cmds (vector<vector<string>> cmd_set, vector<int> &process_ids)
{
    int process_id;
    int p1[2];
    int p2[2];
    int *read_pipe = NULL;
    int *write_pipe = NULL;

    for (int i = cmd_set.size() - 1; i >= 0; i--) {
	vector<string> cmds;

	cmds = cmd_set[i];

	if (pipe(p1)) {
	    errx(-1, "error making pipe");
	}

	read_pipe = p1;

	process_id = add_child(cmds, read_pipe, write_pipe);
	process_ids.push_back(process_id);

	/*
	 * read_pipe is the write pipe for left-side process 
	 * recall A | B | C,  B's read-pipe, becomes A's write
	 */
	p2[0] = p1[0];
	p2[1] = p1[1];
	write_pipe = p2;
    }

    return write_pipe[1];
}

/*
 * Given a string like
 *    'A -a | B -E | C -E'
 *
 * parses the string, makes the subprocess, and connects
 * them all together using pipes to connect A to B to C
 * and C writes to stdout.
 */
int
make_pipe_line (const char *pipe_line_str, vector<int> &process_ids)
{
    vector<vector<string>> cmd_set;
    vector<string> one_cmd;

    vector<string> commands;

    parse_args(pipe_line_str, commands);
    
    if (commands.size() == 0)
	return -1;

    for (unsigned i = 0; i < commands.size(); i++) {
	if (commands[i] == "|") {
	    cmd_set.push_back(one_cmd);
	    one_cmd.clear();
	} else {
	    one_cmd.push_back(commands[i]);
	}
    }
    cmd_set.push_back(one_cmd);

    return make_pipe_line_cmds(cmd_set, process_ids);
}

/*
 * Sets up stdout to write to 'write_fd' and restore when
 * done with command 'fn'
 */
void
wrap_stdout (int write_fd, spawn_func_t fn, void *data)
{
    int save_fd = dup(STDOUT_FILENO);

    dup2(write_fd, STDOUT_FILENO);

    stdout = fdopen(write_fd, "w");

    fn(data);

    fclose(stdout);

    dup2(save_fd, STDOUT_FILENO);
    close(save_fd);

    stdout = fdopen(STDOUT_FILENO, "w");   

    close(write_fd);
}

void
wait_all_process (vector<int> &process_ids)
{
    while (true) {
	// this is more complicated that needed, need
	// to look up how to delete element of vector.
	auto need_wait = false;
	for (auto id : process_ids)
	    if (id) need_wait = true;

	if (!need_wait)
	    break;

	int status;
	int id = waitpid(-1, &status, 0);

	for (unsigned i = 0; i < process_ids.size(); i++) {
	    if (process_ids[i] == id)
		process_ids[i] = 0;
	}
    }
}

#ifdef _TEST

/*
 * Simulate output of command ouput
 */
static void
command_fake (void *data)
{
    static int i = 0;
    
    for (int j = 0; j < 100; j++) {
	printf("bar arg: %d: %d\n", i++, j);
    }
}

int
main (int argc, char **argv)
{
    int write_fd;
    vector<int> process_ids;


    if (argc < 1)
	errx(-1, "Need at least one argument, foo 'abc | def'");

    while (1) {
	process_ids.clear();

	write_fd = make_pipe_line(argv[1], process_ids);
	if (write_fd < 0)
	    break;


	wrap_stdout(write_fd, command_fake, NULL);

	wait_all_process(process_ids);

	printf("\ncontinue> [n|q]");
	fflush(stdout);

	char buf[10];
	char *p;
	if (((p = fgets(buf, sizeof(buf), stdin)) == NULL) || p[0] == 'q') {
	    break;
	}
    }

    exit(0);

    return 0;
}

#endif
