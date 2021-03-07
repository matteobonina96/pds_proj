//
// Created by matteo on 25/02/21.
//

#ifndef PROJE_CLIENTC_H
#define PROJE_CLIENTC_H


#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "filewatcher/FileWatcher.h"
#include "StructureC.h"
#include "../HashC.h"

using namespace boost::asio;
using namespace boost::asio::ip;
namespace pt = boost::property_tree;
#define SIZE_BLOCK 512

class ClientC {

    io_service ioservice;
    tcp::socket client_socket{ioservice};

    std::string user_path;
    HashC hc;
    StructureC sc;

public:

        ClientC(std::string user_p,uint16_t port):user_path(user_p),client_socket(ioservice){

            client_socket
                    .connect(
                            tcp::endpoint(
                                    address::from_string("127.0.0.1"),
                                    port));
            this->handle_connection();
        }

    void connect_handler(const boost::system::error_code &ec);

    void sync_operation(boost::filesystem::path p);

    void sendFile(std::string filename, std::string path, std::string mod);

    void sendLogOut();

    void sendDir(std::string filename, std::string path, std::string mod);

    void fileW();

    void sync_client_to_server();

    void sync_server_to_client();

    void sendFileForSyncro();

    void receiveFileForSyncro();

    void logout(tcp::socket &&socket);

    void sendData_string(const std::string &message);

    std::string getData_string();

    void getData();

    void handle_connection();

    void createDirectory(std::string path);
    void deleteSomething(std::string path);
    void createFile(std::string name, std::string text);
    boost::property_tree::ptree createPtreeDirectory(std::string path,std::string how_to);
    boost::property_tree::ptree createPtreeFile(std::string path,size_t size,std::string how_to);
    void readFile(std::string path, int size);




};
#endif //PROJE_CLIENTC_H
