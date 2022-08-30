#ifndef MYSHELL_PARSE_ARGS_H
#define MYSHELL_PARSE_ARGS_H

#include <vector>
#include <string>
#include <map>

int parse_args (const std::string &s, std::vector<std::string> &args);
bool ends_with(const std::string &value, const std::string &ending);
std::string substitute_var(std::string &arg);

extern std::map<std::string, std::string> global_vars;

#endif //MYSHELL_PARSE_ARGS_H
