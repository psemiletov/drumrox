#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <dirent.h>


#include "utl.h"


using namespace std;

bool file_exists (const string &name)
{
  if (name.empty())
     return false;

  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}


bool ends_with (std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size())
       return false;

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


std::string resolve_symlink (const std::string &path)
{
  bool is_symlink = false;

    struct stat buf;
    int r = stat (path.c_str(), &buf);

    if (S_ISLNK(buf.st_mode))
       is_symlink = true;

    //if (S_ISREG(buf.st_mode)) printf (" stat says file\n");
    if (is_symlink)
       {
        char resolved_fname[FILENAME_MAX];
        int count = readlink (path.c_str(), resolved_fname, sizeof(resolved_fname));
        if (count >= 0)
          {
           resolved_fname[count] = '\0';
           return resolved_fname;
          }
     }

   return path;
}


std::vector <std::string> files_get_list (const std::string &path)
{
  DIR *directory;
  struct dirent *dir_entry;

  std::vector <std::string> result;

  directory = opendir (path.c_str());
  if (! directory)
     return result;

  while ((dir_entry = readdir (directory)))
        {
         std::string t = dir_entry->d_name;

         if (t != "." && t != "..")
             result.push_back (path + "/" + t);
        }

  closedir (directory);

  return result;
}


std::string get_home_dir()
{
  std::string result;

#if !defined(_WIN32) || !defined(_WIN64)

  const char *homedir = getenv ("HOME");

  if (homedir != NULL)
     result = homedir;

#else

  char homeDirStr[MAX_PATH];

  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, homeDirStr)))
    result = homeDirStr;

#endif

  return result;
}


std::string get_file_path (const std::string &path)
{
  size_t i = path.rfind ("/", path.length());

  if (i != std::string::npos)
     return path.substr (0, i);

  return std::string();
}


std::string string_file_load (const string &fname)
{
 if (fname.empty())
    return string();

 std::ifstream t (fname.c_str());
 std::string s ((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());

 return s;
}
