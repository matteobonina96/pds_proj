// Compilation example for GCC (v8 and up), Clang (v7 and up) and MSVC
// g++ -std=c++17 -Wall -pedantic test_fs_watcher.cpp -o test_fs_watcher -lstdc++fs
// clang++ -std=c++17 -stdlib=libc++ -Wall -pedantic test_fs_watcher.cpp -o test_fs_watcher -lc++fs
// cl /W4 /EHsc /std:c++17 /permissive- test_fs_watcher.cpp

#include <iostream>
#include "FileWatcher.h"

void fileW(tcp::socket& socket,std::string user_path) {
    int base = user_path.length();
    FileWatcher fw{user_path,base,std::chrono::milliseconds(5000),socket};

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([] (std::string path_to_watch,int base, FileStatus status,tcp::socket& socket) -> void {
        // Process only regular files, all other file types are ignored
        if(!boost::filesystem::is_regular_file(boost::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
            return;
        }
        int y;
        switch(status) {
            case FileStatus::created:
                std::cout << "File created: " << path_to_watch << '\n';
                y = path_to_watch.length();
                sendFile(socket,path_to_watch.substr(base+1,y),path_to_watch,"new_file");
                break;
            case FileStatus::modified:
                std::cout << "File modified: " << path_to_watch << '\n';
                y = path_to_watch.length();
                sendFile(socket,path_to_watch.substr(base+1,y),path_to_watch,"updated_file");
                break;
            case FileStatus::erased:
                std::cout << "File erased: " << path_to_watch << '\n';
                y = path_to_watch.length();
                sendFile(socket,path_to_watch.substr(base+1,y),path_to_watch,"file_to_delete");
                break;
            default:
                std::cout << "Error! Unknown file status.\n";
        }
    });

}