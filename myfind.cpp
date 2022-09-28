#include <iostream>
#include <cstdlib>
#include <cassert>
#include <unistd.h>
#include <algorithm>
#include <ostream>
#include <iterator>
#include <vector>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>
#include <algorithm>

void printHelp(const std::string& programm_name) {
    std::cout << "Usage: " << programm_name <<  " [-R] [-i] directory filename1 [filename2] ...[filenameN]\n\n";
}

bool isInside(const std::string& str, char c) {
    return str.find(c) != std::string::npos;
}

struct DirCont {
    std::string path;
    std::vector<std::string> files;
    std::vector<std::string> directories;
}; 

struct DirCont* getDirectoryContent(const std::string& path);

bool opt_recursive = false;
bool opt_case_insensitive = false;


int main(int argc, char* argv[]) {
    //std::copy(argv, argv + argc, std::ostream_iterator<char*>(std::cout, "\n"));

    char opt_char;
    char* program_name = argv[0];

    unsigned short opt_cnt_i = 0;
    unsigned short opt_cnt_R = 0;

    std::vector<std::string> args(argv + 1, argv + argc);

    std::string path;
    std::vector<std::string> files;

    for (const auto& arg : args) {
        std::cout << arg << std::endl;
    }

    if(argc == 1) {
        printHelp(program_name);
        exit(1);
    }

    // Parse command line args
    while ((opt_char = getopt(argc, argv, "iR")) != EOF) {
        switch (opt_char) {
            case '?':
                std::cerr << program_name << ": Error - Unknown option\n";
                printHelp(program_name);
                exit(1);
            case 'i': //case-insensitive search
                opt_cnt_i++;
                opt_case_insensitive = true;
                break;
            case 'R': //enable recursive search
                opt_cnt_R++;
                opt_recursive = true;
                break;
            default:
                assert("An unknown error occured." && 0);
        }
    }

    if (opt_cnt_R > 1 || opt_cnt_i > 1){
        std::cerr << program_name << ": Error - Multiple usage of options.\n";
        exit(1);
    }

    //Prune options out of arg vector

    for (auto arg : args) {
        if (arg.at(0) != '-'){
            files.push_back(arg);
        }
    }

    //check for empty files
    if (!files.empty()){
        path = files[0];
        files.erase(files.begin());
    } else {
        std::cerr << program_name << ": Error: No target files specified\n";
        exit(1);
    }

    //convert files to lowercase
    if (opt_case_insensitive){
        for(int j = 0; j < files.size(); j++){
            std::transform(files.at(j).begin(), files.at(j).end(), files.at(j).begin(), [](unsigned char c){ return std::tolower(c);});
        }
    }

    //check dir pattern
    if (!isInside(path, '/')){
        std::cerr << program_name << ": Error: " << path << " is not a directory.\n";
        printHelp(program_name);
        exit(1);
    }

    //std::cout << "Path: " << path << std::endl;
    //for (const auto& file: files) {
    //    std::cout << file << std::endl;
    //}

    //fork and searach for files

    //list directory -> iterate through
    struct DirCont* entries = getDirectoryContent(path);

    if (std::find(entries->files.begin(), entries->files.end(), files[0]) != entries->files.end()){
        std::cout << "Element found!\n";
        //std::cout << entries->path << "/" << files[0] << std::endl;
    } else {
        std::cout << "Element not found\n";
    }



    delete entries;    
    return 0;
}

struct DirCont* getDirectoryContent(const std::string& path){
    
    struct dirent *direntp;
    struct DirCont* dircont = new DirCont;
    DIR *dirp;

    if ( (dirp = opendir(&(path[0]))) == NULL) {
        std::cerr << "Failed to open directory!\n";
        exit(1);
    }

    // char cwd_buf[255];
    // if (getcwd(cwd_buf, 255) == NULL) {
    //     std::cerr << "Failed to get cwd.\n";
    //     exit(1);
    // }
    //dircont->path = std::string(cwd_buf);

    while ((direntp = readdir(dirp)) != NULL) {
        if(direntp->d_type == DT_REG){
            dircont->files.push_back(std::string(direntp->d_name));        
        } else if (direntp->d_type == DT_DIR){
            if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0){
                dircont->directories.push_back(std::string(direntp->d_name));
            }
        } 
    }
    if(opt_case_insensitive){
        for(int j = 0; j < dircont->files.size(); j++){
            std::transform(dircont->files.at(j).begin(), dircont->files.at(j).end(), dircont->files.at(j).begin(), [](unsigned char c){ return std::tolower(c);});
        }
    }
    while ((closedir(dirp) == -1) && (errno == EINTR))
        ;
    

    return dircont;
}
