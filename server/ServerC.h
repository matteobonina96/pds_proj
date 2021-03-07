//
// Created by matteo on 25/02/21.
//

#ifndef PROJE_SERVERC_H
#define PROJE_SERVERC_H
#include <boost/asio.hpp>
#include <iostream>
#include <sqlite3.h>
#include <thread>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include "ServerStructureC.h"
#include "users.h"
#include "../HashC.h"


using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
using boost::asio::ip::tcp;
namespace pt = boost::property_tree;

#define SIZE_BLOCK 512



class ServerC {
    tcp::socket server_socket;
    sqlite3* DB; //dichiaro un database sqlite3
    std::string up_select; //Stringa percorso cartella QUERY
    HashC hc;
    ServerStructureC sc;
    std::mutex mutex_db;






public:
    ServerC(boost::asio::io_service& io_service)
    : server_socket(io_service)
            {
            }

    tcp::socket& socket()
    {
        return server_socket;
    }

    void start();

    void sendData(const string& message);
    string getData();
    int getData2(std::string user_path);
    std::string iscrizione();
    std::string login();
    void logout(tcp::socket&& socket);
    void sync_client_to_server(std::string dir);
    void sync_server_to_client(std::string dir);
    void sendFileForSyncro(std::string user_path);
    void receiveFileForSyncro(std::string user_path);

    void start_server_config(boost::filesystem::path p);

    void getDataForServerToClientSync(std::string user_path);
    //static int callback(void* data, int argc, char** argv, char** azColName);

    void createDirectory(std::string path,std::string user_path);

    void createNewDir(std::string path);
    void deleteSomething(std::string path) ;
    void createFile(std::string user_path,std::string path, std::string text) ;

    boost::property_tree::ptree createPtreeDirectory(std::string path,std::string how_to);

    boost::property_tree::ptree createPtreeFile(std::string path,size_t size,std::string how_to);
    void sendFile(std::string filename,std::string path,std::string mod) ;

    void sendDir(std::string filename,std::string path,std::string mod);
    void readFile(std::string user_path, std::string path, int size);



};


#endif //PROJE_SERVERC_H
