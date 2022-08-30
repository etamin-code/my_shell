#ifndef MYRLS_RECURSIVE_LS_H
#define MYRLS_RECURSIVE_LS_H

int recursive_ls(const char *path);
int fn(const char * path, const struct stat * sb, int fileflags, struct FTW *pftw);
std::string get_info(const struct stat * sb);
bool comparator(const std::tuple<std::string, std::string> &f1,
                const std::tuple<std::string, std::string> &f2);

#endif //MYRLS_RECURSIVE_LS_H
