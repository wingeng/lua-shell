#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <string>
#include <vector>

int parse_args(const char *str, std::vector<std::string> &toks);

void vsdump(std::vector<std::string>);

#endif
