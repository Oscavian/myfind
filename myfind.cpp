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



void printHelp(const std::string& programm_name) {
    std::cout << "Usage: " << programm_name <<  " [-R] [-i] directory filename1 [filename2] ...[filenameN]\n\n";
}

bool isInside(const std::string& str, char c) {
    return str.find(c) != std::string::npos;
}

struct DirCont {
    std::vector<std::string> files;
    std::vector<std::string> directories;
}; 

std::string printPath(std::string searchedFile, std::string& path);

//normalSearch = getDirectoryContent, just renamed for coherency
struct DirCont* normalSearch(std::string& path);
void recursiveSearch(std::string searchedFile, std::string path, struct DirCont* dircont);

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

    int status;
    pid_t wpid;

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

    //filesToSearch notes how many files should be searched aka. child processes created
    int filesToSearch = files.size();
    pid_t pids[filesToSearch];

    for(int i = 0; i < filesToSearch; i++){
        pids[i] = fork();

        switch (pids[i]){
            case -1: /* error */
                fprintf(stderr, "error when forking child process\n");
                exit(1);
            case 0: /* child process */
                 if(opt_recursive){
                    struct DirCont* dircont = new DirCont;
                    recursiveSearch(files[i], path, dircont);
                    delete dircont;
                } else {
                    struct DirCont* entries = normalSearch(path);
                    //loop important as more than 1 matching file can be found with case_insensitivity
                    //eg. searchedFile = test.txt, matching files = test.txt, Test.txt, TEST.txt usw.
                    if(opt_case_insensitive){
                        while(1){
                            //position is iterator on matching entry
                            auto position = std::find(entries->files.begin(), entries->files.end(), files[i]);
                            //position != entries->files.end() means searched entry exists in vector 
                            if (position != entries->files.end()){
                                std::cout << (int)getpid() << ":" << files[i] << ":" << (std::string)printPath(files[i], path) << "\n";
                                entries->files.erase(position);   
                            } else {
                                /////////std::cout << "No (more) elements matching " << files[i] << " in directory " << path << "\n";
                                entries->files.clear();
                                break;
                            } 
                        }
                    }else{
                        auto position = std::find(entries->files.begin(), entries->files.end(), files[i]);
                            //position != entries->files.end() means searched entry exists in vector 
                         if (position != entries->files.end()){
                            std::cout << (int)getpid() << ": " << files[i] << ": " << (std::string)printPath(files[i], path) << "\n";
                            entries->files.erase(position);   
                        } else {
                            entries->files.clear();   
                        } 
                    }
                    //Probleme: 
                    //Bei printPath() mit gesetzter Option -i wird die gefundene Datei am Ende des Pfades immer klein geschrieben,
                    //da alles auf lowercase gesetzt wird, um zu vergleichen
                    //wenn die Datei so angegeben werden soll, wie sie wirklich heißt,
                    //müssen wir den nicht-lowercase Namen zwischenspeichern 

                    delete entries;  
                } 
                exit(0);
            default: /* parent */  
            ;  
        }
    }

    for(int i = 0; i < filesToSearch; i++){
        wpid = wait(&status);
        if(wpid == -1){
            fprintf(stderr, "error while waitting for child process to finish\n");
        }
        if (!WIFEXITED(status)){
            fprintf(stderr, "Child process has finished with error or via signal, status: %d\n", WEXITSTATUS(status));
        }
    } 
    return 0;
}

struct DirCont* normalSearch(std::string& path){
    struct dirent *direntp;
    DIR *dirp;
    struct DirCont* dircont = new DirCont();

    if ((dirp = opendir(&(path[0]))) == NULL) {
        std::cerr << "Failed to open directory!\n";
        exit(1);
    }

    while ((direntp = readdir(dirp)) != NULL) {
        if(direntp->d_type == DT_REG){
            dircont->files.push_back(std::string(direntp->d_name));       
        } else if (direntp->d_type == DT_DIR){
            if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0){
                dircont->directories.push_back(std::string(direntp->d_name));
            }
        } 
    }
    while ((closedir(dirp) == -1) && (errno == EINTR))
        ;

    if(opt_case_insensitive){
        for(int j = 0; j < dircont->files.size(); j++){
            std::transform(dircont->files.at(j).begin(), dircont->files.at(j).end(), dircont->files.at(j).begin(), [](unsigned char c){ return std::tolower(c);});
        }
    }
    return dircont;
}

void recursiveSearch(std::string searchedFile, std::string path, struct DirCont* dircont){
    struct dirent *direntp;
    DIR *dirp;

    if ((dirp = opendir(&(path[0]))) == NULL) {
        std::cerr << "Failed to open directory!\n";
        exit(1);
    }
    //used below to check if subdirs have been added, bc that means current dir has childdirs, no backing out to parentdir yet
    int dirInVec = dircont->directories.size();

    while ((direntp = readdir(dirp)) != NULL) {
        if(direntp->d_type == DT_REG){
            dircont->files.push_back(std::string(direntp->d_name));     
        } else if (direntp->d_type == DT_DIR){
            if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0){
                //dirs are inserted at front of vec, so that newest dirs (subdirs) are first looked at below, important for keeping the hierarchy of dirs intact
                dircont->directories.insert(dircont->directories.begin(), std::string(direntp->d_name));
            }
        } 
    }
    while ((closedir(dirp) == -1) && (errno == EINTR))
        ;

    if(opt_case_insensitive){
        for(int j = 0; j < dircont->files.size(); j++){
            std::transform(dircont->files.at(j).begin(), dircont->files.at(j).end(), dircont->files.at(j).begin(), [](unsigned char c){ return std::tolower(c);});
        }
        while(1){
            //loop important as more than 1 matching file can be found with case_insensitivity
            auto position = std::find(dircont->files.begin(), dircont->files.end(), searchedFile);
            if (position != dircont->files.end()){
                std::cout << (int)getpid() << ": " << searchedFile << ": " << (std::string)printPath(searchedFile, path) << "\n";
                dircont->files.erase(position);   
            } else {
                //clear files as all other found files are irrelevant and needn't be saved
                dircont->files.clear();
                break;
            } 
        }
    }else{
        auto position = std::find(dircont->files.begin(), dircont->files.end(), searchedFile);
        if (position != dircont->files.end()){
            std::cout << (int)getpid() << ":" << searchedFile << ":" << (std::string)printPath(searchedFile, path) << "\n";
            dircont->files.erase(position);   
        } else {
            dircont->files.clear();
        } 
    }
    do{     
        //if compares count of dirs in vec before reading current dir and after, if size has changed, that means current dir has subdirs
        //if current dir has no subdirs, return to parentdir
        if(dirInVec == dircont->directories.size()){
            return;
        }
        else{
            //get the first subdirectory in the vector = next dir to read
            std::string nextDir = dircont->directories.front();
            //then immediately delete it from the vector to prevent double-checking
            dircont->directories.erase(dircont->directories.begin());
            recursiveSearch(searchedFile, path + "/" + nextDir, dircont);
        }
    //keep going until vec of dirs empty, no more dirs to check
    }while(!dircont->directories.empty());
    return;
}

std::string printPath(std::string searchedFile, std::string& path){
    if(path[0] != '/'){
        //if path is relative
        char *relativepath = new char[path.length() + 1];
        //string path has to be converted to char* for realpath()
        strcpy(relativepath, path.c_str());
        if(relativepath == NULL){
            std::cerr << "error while transforming string to char*" << std::endl;
            exit(1);
        }

        char absolutepath [PATH_MAX+1];
        char *ptr;
        //finds absolute path from relative, saves in absolutepath
        if((ptr = realpath(relativepath, absolutepath)) == NULL){
            std::cerr << "Finding absolute path not possible" << std::endl;
            delete relativepath;
            exit(1);
        };
        delete relativepath;
        
        //char* to string for return value
        std::string myabsp(ptr);
        return myabsp + "/" + searchedFile;
        
    } else {
        //if path is absolute
        if(path.length() == 1){
            //if path == root == /
            return "Root/" + searchedFile;
        }else{
           return path + "/" + searchedFile;
        } 
    }
}