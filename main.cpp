#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <algorithm>
#include <ostream>
#include <iterator>
#include <vector>
#include <sys/wait.h>
#include <sys/types.h>

void printHelp(std::string programm_name) {
    std::cout << "Usage: " << programm_name <<  " [-R] [-i] directory filename1 [filename2] ...[filenameN]\n\n";
    return;
}

bool isInside(const std::string& str, char c) {
    return str.find(c) != std::string::npos;
}

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

    for (auto arg : args) {
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
    ///TODO
    for (auto arg : args) {
        if (arg.at(0) != '-'){
            files.push_back(arg);
        }
    }

    path = files[0];
    files.erase(files.begin());

    if (!isInside(path, '/')){
        std::cerr << program_name << ": Error: " << path << " is not a directory.\n";
        printHelp(program_name);
    }

    std::cout << "Path: " << path << std::endl;
    for (auto file: files) {
        std::cout << file << std::endl;
    }

    //fork and searach for files
    ///TODO



    return 0;
}

