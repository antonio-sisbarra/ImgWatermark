#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <unistd.h>

#define GetCurrentDir getcwd

//Checks the correctness of filename params
bool checkParams(const char *markFileName, const char *inpPath, const char *outPath){
    struct stat info;

    if(stat( markFileName, &info ) != 0){
        std::cerr << "Cannot access to file\n";
        return false; //cannot access to file
    }

    if( (info.st_mode & S_IFDIR) ){
        std::cerr << "Mark is a directory\n";
        return false; //is a directory
    }

    if(stat( inpPath, &info ) != 0){
        std::cerr << "Cannot access to directory input\n";
        return false; //cannot access to directory
    }
        
    if( !(info.st_mode & S_IFDIR) ){
        std::cerr << "It is not a directory\n";
        return false; //is not a directory
    }
    return true;
}

//Extract filename without extension
std::string getOnlyFilename(const std::string &filewithformat){
    size_t position = filewithformat.find(".");
    std::string extractName = (std::string::npos == position)? filewithformat : filewithformat.substr(0, position);
    return extractName;
}

//Returns the current directory of the program
std::string GetCurrentWorkingDir( void ) {
  char buff[FILENAME_MAX];
  if(!GetCurrentDir( buff, FILENAME_MAX ))
    return "";
  std::string current_working_dir(buff);
  return current_working_dir;
}