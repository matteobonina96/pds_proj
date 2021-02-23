#pragma once

#include <chrono>
#include <boost/filesystem.hpp>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include <boost/asio.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
// Define available file changes test
enum class FileStatus {directory_created,created, modified, erased};

class FileWatcher {
public:
    std::string path_to_watch;
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;
    tcp::socket& socket;
    int base;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch,int base, std::chrono::duration<int, std::milli> delay,tcp::socket& socket) : path_to_watch{path_to_watch}, delay{delay}, socket{socket}, base{base}{
        for(auto &file : boost::filesystem::recursive_directory_iterator(path_to_watch)) {
            paths_[file.path().string()] = boost::filesystem::last_write_time(file);
        }
    }

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void (std::string,int, FileStatus,tcp::socket&)> &action) {
        while(running_) {
            // Wait for "delay" milliseconds
            std::this_thread::sleep_for(delay);

            auto it = paths_.begin();
            while (it != paths_.end()) {
                if (!boost::filesystem::exists(it->first)) {
                    action(it->first,base, FileStatus::erased,socket);
                    it = paths_.erase(it);
                }
                else {
                    it++;
                }
            }

            // Check if a file was created or modified
            for(auto &file : boost::filesystem::recursive_directory_iterator(path_to_watch)) {
                auto current_file_last_write_time = boost::filesystem::last_write_time(file);
                if(!contains(file.path().string())) {
                    cout<<"SOMETHING CHANGES!\n";
                    if(boost::filesystem::is_directory(file.path().string())) {
                        paths_[file.path().string()] = current_file_last_write_time;
                        action(file.path().string(),base, FileStatus::directory_created,socket);
                    }
                    else {
                        paths_[file.path().string()] = current_file_last_write_time;
                        action(file.path().string(),base, FileStatus::created,socket);
                    }

                    // File modification
                } else {
                    if(paths_[file.path().string()] != current_file_last_write_time) {
                        paths_[file.path().string()] = current_file_last_write_time;
                        action(file.path().string(),base, FileStatus::modified,socket);
                    }
                }
            }
        }
    }
private:
    std::unordered_map<std::string, std::time_t> paths_;
    bool running_ = true;

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key) {
        auto el = paths_.find(key);
        return el != paths_.end();
    }};