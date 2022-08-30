#ifndef MYSHELL_BUILTINS_H
#define MYSHELL_BUILTINS_H

#include <vector>
#include <string>
#include <map>

int execute_command(std::vector<std::string> &parsed);
int check_and_execute(const std::string &input);
bool compare_wildcard(std::string str, std::string pattern);
std::vector<std::string> get_wildcard_files(const std::string& search_path);

int merrno(std::vector<std::string> &args);
int mpwd(std::vector<std::string> &args);
int mcd(std::vector<std::string> &args);
int mexit(std::vector<std::string> &args);
int mecho(std::vector<std::string> &args);
int mexport(std::vector<std::string> &args);
int execute_script(std::vector<std::string> &args);

typedef int (*FnPtr)(std::vector<std::string> &);
extern std::map<std::string, FnPtr> builtin_programs;
extern std::vector<std::string> history;
extern int error_value;
extern std::map<std::string, std::string> global_vars;

#endif //MYSHELL_BUILTINS_H
