#ifndef UTL_H
#define UTL_H

#include <string>
#include <vector>

bool file_exists (const std::string &name);
bool ends_with (std::string const &value, std::string const & ending);
std::string resolve_symlink (const std::string &path);
std::string get_home_dir();

std::vector <std::string> files_get_list (const std::string &path);
std::string get_file_path (const std::string &path);
std::string string_file_load (const std::string &fname);

#endif
