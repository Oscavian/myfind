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
    std::vector<std::string> files;
    std::vector<std::string> directories;
}; 

void printPath(std::string searchedFile, std::string& path);

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

    std::string searchedFile = files[0];

    if(opt_recursive){
        struct DirCont* dircont = new DirCont;
        recursiveSearch(searchedFile, path, dircont);
        delete dircont;
    } else {
        struct DirCont* entries = normalSearch(path);

        //loop lists all found files corresponding with searchedFile
        //especially important for searches with -i as find() only gets the first entry of a matching file
        //and would skip others eg. searchedFile = test.txt, matching files = test.txt, Test.txt, TEST.txt usw.
        while(1){
            //position is iterator on matching entry
            auto position = std::find(entries->files.begin(), entries->files.end(), searchedFile);
            //position != entries->files.end() means searched entry exists in vector 
            if (position != entries->files.end()){
                std::cout << "Element found!\n";
                printPath(searchedFile, path);
                entries->files.erase(position);   
            } else {
                ///////////std::cout << "No more elements matching " << searchedFile << " in this directory\n";
                entries->files.clear();
                break;
            } 
        }
    
        //Probleme: 
        //Bei printPath() mit gesetzter Option -i wird die gefundene Datei am Ende des Pfades immer klein geschrieben,
        //da alles auf lowercase gesetzt wird, um zu vergleichen
        //wenn die Datei so angegeben werden soll, wie sie wirklich heißt,
        //müssen wir den nicht-lowercase Namen zwischenspeichern 

        delete entries;  
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
             //std::cout << direntp->d_name << " is file\n";     
        } else if (direntp->d_type == DT_DIR){
            if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0){
                dircont->directories.push_back(std::string(direntp->d_name));
                //std::cout << direntp->d_name << " is dir\n"; 
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

    ///////////std::cout << "Path to be looked at: " << path << std::endl;
    if ((dirp = opendir(&(path[0]))) == NULL) {
        std::cerr << "Failed to open directory!\n";
        exit(1);
    }

    //used below to check if subdirs have been added = current dir has childdirs, no backing out to parentdir yet
    int dirInVec = dircont->directories.size();

    while ((direntp = readdir(dirp)) != NULL) {
        if(direntp->d_type == DT_REG){
            dircont->files.push_back(std::string(direntp->d_name)); 

             ///////////std::cout << direntp->d_name << " is file\n";     
        } else if (direntp->d_type == DT_DIR){
            if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0){
                ///////////dircont->directories.push_back(std::string(direntp->d_name));

                //dirs are inserted at front of vec, so that newest dirs (subdirs) are first looked at below
                //important for keeping the hierarchy of dirs intact
                dircont->directories.insert(dircont->directories.begin(), std::string(direntp->d_name));

                ///////////std::cout << direntp->d_name << " is dir\n"; 
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
    while(1){
        auto position = std::find(dircont->files.begin(), dircont->files.end(), searchedFile);
        if (position != dircont->files.end()){
            std::cout << "Element found!\n";
            printPath(searchedFile, path);
            dircont->files.erase(position);   
        } else {
            ///////////std::cout << "No more elements matching " << searchedFile << " in this directory\n";
            //clear files as all other found files are irrelevant and needn't be saved
            dircont->files.clear();
            break;
        } 
    }

    do{
        ///////////std::cout << "\ndirs before: " << dirInVec << " dirs after search: " << dircont->directories.size() << "\n";
        
        //if compares dirs in vec before readind dir and after
        //if size has changed, that means current dir has subdirs
        //if current dir has no subdirs, return to parentdir
        if(dirInVec == dircont->directories.size()){
            return;
        }
        else{
            //get the first subdirectory in the vector = next dir to read
            std::string nextDir = dircont->directories.front();

            ///////////std::cout << "\nnext dir: " << nextDir << "\n";

            //then immediately delete it from the vector to prevent double-checking
            dircont->directories.erase(dircont->directories.begin());

            ///////////for(std::string i : dircont->directories) 
                ///////////std::cout << "Dir in list: " << i << "\n" << std::endl;

            recursiveSearch(searchedFile, path + "/" + nextDir, dircont);
        }
    //keep going until vec of dirs empty, no more dirs to check
    }while(!dircont->directories.empty());
    ///////////std::cout << "No more directories to check\n";
    return;
}

void printPath(std::string searchedFile, std::string& path){
    if(path[0] != '/'){
        //wenn der Pfad relativ ist
        char *relativepath = new char[path.length() + 1];
        //string path muss zu char* konvertiert werden für realpath()
        strcpy(relativepath, path.c_str());
        if(relativepath == NULL){
            std::cerr << "Transforming from string to char* did not work" << std::endl;
            exit(1);
        }
        //std::string myrelp(relativepath);
        //std::cout << "Relative Path:" << myrelp << "/" << searchedFile << std::endl;

        char absolutepath [PATH_MAX+1];
        char *ptr;
        //findet den absoluten Pfad von einem relativen, speichert in absolutepath
        if((ptr = realpath(relativepath, absolutepath)) == NULL){
            std::cerr << "Finding absolute path not possible" << std::endl;
            delete relativepath;
            exit(1);
        };
        delete relativepath;
        
        //char* zu string für einfacheres Ausgeben
        std::string myabsp(ptr);

        std::cout << "Path:" <<  myabsp << "/" << searchedFile << std::endl;
        
    } else {
        //wenn der Pfad absolut ist
        if(path.length() == 1){
            //wenn man in root sucht (path == /)
            std::cout << "Path: Root/" << searchedFile << std::endl;
        }else{
           std::cout << "Path:" << path << "/" << searchedFile << std::endl; 
        } 
    }
}