#ifndef SPAWN_H
#define SPAWN_H

#include <string>
#include <vector>

typedef void (*spawn_func_t)(void *);

int make_pipe_line(const char *pipe_line_str,
		   std::vector<int> &process_ids);

int make_pipe_line_cmds(std::vector<std::vector<std::string>> cmd_set,
			std::vector<int> &process_ids);

void wrap_stdout(int write_fd, spawn_func_t func, void *data);

void wait_all_process(std::vector<int> &process_ids);

#endif
