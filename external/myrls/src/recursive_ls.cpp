#include <iostream>
#include <ftw.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <tuple>
#include <algorithm>
#include <map>
#include "recursive_ls.h"

std::vector<std::string> directories;
std::map<std::string, std::vector<std::tuple<std::string, std::string>>> files_in_dirs;


int recursive_ls(const char *path) {
    int res = nftw(path, fn, 30, FTW_MOUNT | FTW_PHYS);
    if (res) {
        std::cerr << "Error while recursion" << std::endl;
        return -1;
    }

    for (const auto &dir : directories) {
        std::vector<std::tuple<std::string, std::string>> files = files_in_dirs[dir];
        std::sort(files.begin(), files.end(), comparator);
        std::cout << std::endl;
        std::cout << dir << ":" <<std::endl;

        for (const auto &tpl : files) {
            std::string fpath, fdesc;
            std::tie(fpath, fdesc) = tpl;

            std::cout << fdesc << std::endl;
        }
    }
    return 0;
}


int fn(const char * path, const struct stat * sb, int typeflag, struct FTW * ftwbuf) {
    std::string info = get_info(sb);

    std::string fpath = path;
    std::string fname = fpath.substr(fpath.find_last_of("/\\") + 1);

    bool is_dir = false;

    std::string prefix;
    std::string postfix;
    if (typeflag == FTW_DNR || typeflag == FTW_SLN || typeflag == FTW_NS) {
        std::cerr << "Unable to access " << path << std::endl;
        return 0;
    } else if (S_ISDIR(sb->st_mode)) {
        prefix = "/";
        is_dir = true;
    } else if (S_ISLNK(sb->st_mode)) {
        prefix = "@";
        char buf[100];
        ssize_t len = ::readlink(path, buf, sizeof(buf)-1);
        buf[len] = '\0';
        postfix = "->" + std::string(buf);
    } else if (S_ISFIFO(sb->st_mode)) {
        prefix = "|";
    } else if (S_ISSOCK(sb->st_mode)) {
        prefix = "=";
    } else if ((sb->st_mode & S_IEXEC) != 0) {
        prefix = "*";
    } else if (S_ISREG(sb->st_mode)){
        prefix = "";
    } else {
        prefix = "?";
    }

    std::string description =  get_info(sb) + " " + prefix + fname + postfix;
    std::tuple<std::string, std::string> ls_info = std::make_tuple(fpath, description);
    if (is_dir) {
        directories.emplace_back(fpath);
    }
    std::string dir_prefix = fpath.substr(0, fpath.find_last_of('/'));
    files_in_dirs[dir_prefix].emplace_back(ls_info);
    return 0;
}


std::string get_info(const struct stat * sb){
    //permission-----------------------------------
    std::string permission(9, '-');
    permission[0] = (sb->st_mode & S_IRUSR) ? 'r' : '-';
    permission[1] = (sb->st_mode & S_IWUSR) ? 'w' : '-';
    permission[2] = (sb->st_mode & S_IXUSR) ? 'x' : '-';
    permission[3] = (sb->st_mode & S_IRGRP) ? 'r' : '-';
    permission[4] = (sb->st_mode & S_IWGRP) ? 'w' : '-';
    permission[5] = (sb->st_mode & S_IXGRP) ? 'x' : '-';
    permission[6] = (sb->st_mode & S_IROTH) ? 'r' : '-';
    permission[7] = (sb->st_mode & S_IWOTH) ? 'w' : '-';
    permission[8] = (sb->st_mode & S_IXOTH) ? 'x' : '-';

    //username--------------------------------------
    struct passwd *p;
    uid_t  uid = sb->st_uid;

    if ((p = getpwuid(uid)) == NULL) {
        return "";
    }
    std::string username = p->pw_name;
    int u_len = username.size();
    for (int i = 0; i < 15 - u_len; i++) {
        username += " ";
    }

    //size-------------------------------------------
    std::string size = std::to_string(sb->st_size);
    int s_len = size.size();
    for (int i = 0; i < 12 - s_len; i++) {
        size += " ";
    }

    //time-------------------------------------------
    struct tm *tm;
    char buf[200];
    tm = localtime(&sb->st_mtime);
    strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", tm);
    std::string time = buf;

    return permission + " " + username + " " + size + " " + time;
}


bool comparator(const std::tuple<std::string, std::string> &f_1,
                const std::tuple<std::string, std::string> &f_2){
    std::string path_1, desc_1, path_2, desc_2;

    std::tie(path_1, desc_1) = f_1;
    std::tie(path_2, desc_2) = f_2;

    size_t min_size = std::min(path_1.size(), path_2.size());
    for (size_t i = 0; i < min_size; i++ ) {
        if (path_1[i] < path_2[i]) {
            return true;
        }
        if (path_1[i] > path_2[i]) {
            return false;
        }
    }

    if (min_size == path_1.size()) {
        return true;
    }
    return false;
}

