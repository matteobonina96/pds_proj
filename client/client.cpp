
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "ClientStructureManage.cpp"
#include "filewatcher/FileWatcher.h"
#include "FileManageClient.cpp"



using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

void sync_operation(boost::filesystem::path p);
void sendFile(tcp::socket& socket,std::string filename,std::string path,std::string mod);
void sendDir(tcp::socket& socket,std::string filename,std::string path,std::string mod);
void fileW(tcp::socket& socket,std::string user_path);
void sync_client_to_server(tcp::socket& client_socket,std::string user_path);
void sync_server_to_client(tcp::socket& client_socket,std::string user_path);
void sendFileForSyncro(tcp::socket& client_socket, std::string user_path);
void receiveFileForSyncro(tcp::socket& client_socket, std::string user_path);


void sendData(tcp::socket& socket, const string& message)
{
    write(socket,
          buffer(message + "\n"));
}




string getData(tcp::socket& socket)
{
    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\n");
    string data = buffer_cast<const char*>(buf.data());
    return data;
}



void getData2(tcp::socket& socket,std::string user_path) {
    std::array<char, 512> resp;
    memset(&resp,0,resp.size());
    cout<<"SBEMMMMMMMMMMMMMM"<<std::endl;
    boost::asio::read(socket, buffer(resp,512));
    cout<<"SBEMMMMdsdwdwMMMMMMMMMM"<<std::endl;

    std::string s(resp.data());
    std::stringstream ss(s);
    boost::property_tree::ptree root;
    std::cout <<"JSON from server: "<<std::endl<<s<< std::endl;
    boost::property_tree::json_parser::read_json(ss, root);

    std::string type = root.get<std::string>("type");

    if (type == "directory") { // nel json ho ricevuto info su una directory
        createDirectory(user_path+"/"+root.get<std::string>("path"));

    } else if (type=="file" ){ // nel json ho ricevuto info su un file

        std::string path = root.get<std::string>("path");
        size_t size = (root.get<size_t>("size"));
        readFile(socket,user_path, path, size);
    }
    else if(type=="server_needs_for_sync") {
        //devo mandare una serie di file al server
        std::vector<std::pair<std::string, std::string>> fileNames;
        // Iterator over all fileNames
        root.get_child("fileNames").size();
        for (boost::property_tree::ptree::value_type &file : root.get_child("fileNames"))
        {   //in first troviamo se DIRECTORY o FILE
            std::string type = file.first;
            //in second troviamo il nome del file
            std::string fileName = file.second.data();
            fileNames.push_back(std::make_pair(type, fileName));
        }
        for (std::pair<std::string, std::string> n : fileNames) {
            if(n.first == "directory")
                sendDir(socket,n.second,user_path+"/"+n.second,"new_dir");
        }
        for (std::pair<std::string, std::string> n : fileNames) {
            if (n.first == "file")
                sendFile(socket, n.second, user_path + "/" + n.second, "new_file");
        }
    }
}




int main(int argc, char* argv[])
{
    if(argc!=2)  {
        cout<<"Devi specificare il path da monitorare..."<<std::endl;
        return 1;
    }
    std::string user_path = argv[1];

    io_service io_service;
    // socket creation
    ip::tcp::socket client_socket(io_service);

    client_socket
            .connect(
                    tcp::endpoint(
                            address::from_string("127.0.0.1"),
                            9999));

    // Getting username from user

    string reply, response,scelta_from_s;

    while (true) {
        //SCELTA
        response = getData(client_socket);
        response.pop_back();
        cout << "Server: " << response << endl;
        if(response=="start_config") break;
        //mando
        getline(cin, reply);
        sendData(client_socket, reply);

    }

    scelta_from_s = getData(client_socket);
    scelta_from_s.pop_back();
    cout << "Scelta: " << scelta_from_s << endl;

        if(scelta_from_s=="1") //classic
        {
            sync_client_to_server(client_socket, user_path);
        }

    if(scelta_from_s=="2") //devo fare backup , prendo quello che mi manda e salvo
    {
        sync_server_to_client(client_socket,user_path);

    }
    cout<<"Something changed!";
    //lancio file watcher in background
    fileW(client_socket,user_path);


    return 0;
}


void sync_operation(boost::filesystem::path p) {
    std::string digest;
    std::string name;

    try
    {
        if (exists(p))
        {
            if (is_regular_file(p))
                cout << p << " size is " << file_size(p) << '\n';

            else if (is_directory(p))
            {
                int x = p.string().length();
                std::vector<path> v;
                int s;
                char* outputBuf;
                json structure;
                structure["entries"]=json::array();
                json row;
                for (recursive_directory_iterator it(p), end; it != end; ++it) {
                    if(is_directory(it->path()))
                        row["type"] = "directory";
                    else
                        row["type"]="file";

                    int y = it->path().string().length();
                    row["path"] = it->path().string().substr(x+1 ,y); //+1 perche parte da user_path/ ( lo slash lo metto con +1 )
                    row["hash"] = get_hash_file(it->path());
                    row["timestamp_last_mod"] = boost::filesystem::last_write_time(it->path());

                    structure["entries"].push_back(row);
                    v.push_back(it->path());
                }
                write_sync_structure_client(structure);
                std::sort(v.begin(), v.end());

                /*for (auto&& x : v) {
                    cout << "   # " << x.filename() << '\n';
                }*/
            }
            else
                cout << p << " exists, but is not a regular file or directory\n";
        }
        else
            cout << p << " does not exist\n";
    }

    catch (const filesystem_error& ex)
    {
        cout << ex.what() << '\n';
    }
}

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

void sync_server_to_client(tcp::socket& client_socket,std::string user_path) {
    sync_operation(user_path);
    //mando client struct
    sendFile(client_socket,"client-struct.json","utility/client-struct.json","start_config");

        // ricevo server-struct
        getData2(client_socket,user_path);

        receiveFileForSyncro(client_socket,user_path);




}

void sync_client_to_server(tcp::socket& client_socket,std::string user_path) {
    //Creo le struct per la sincronizazzione
    sync_operation(user_path);

        //ricevo server-struct
        getData2(client_socket,user_path);

        //invio client-struct
        sendFile( client_socket,"client-struct.json","utility/client-struct.json","start_config");

        //Mando i file al server
        sendFileForSyncro(client_socket,user_path);

    }

    void receiveFileForSyncro(tcp::socket& client_socket, std::string user_path) {
        boost::property_tree::ptree root = confronto_sync_server_to_client(user_path);

        for(int j=0;j<root.get_child("fileNames").size();j++) {
            getData2(client_socket,user_path);
        }
}

    void sendFileForSyncro(tcp::socket& client_socket, std::string user_path) {
        //Ottengo un tree con i file da inviare, ottenuto da confronto tra server/client struct
        boost::property_tree::ptree root = confronto_sync_client_to_server(user_path);

        //
        std::vector<std::pair<std::string, std::string>> fileNames;
        // Iterator over all fileNames
        root.get_child("fileNames").size();
        for (boost::property_tree::ptree::value_type &file : root.get_child("fileNames"))
        {   //in first troviamo se DIRECTORY o FILE
            std::string type = file.first;
            //in second troviamo il nome del file
            std::string fileName = file.second.data();
            fileNames.push_back(std::make_pair(type, fileName));
        }
        for (std::pair<std::string, std::string> n : fileNames) {
            if(n.first == "directory")
                sendDir(client_socket,n.second,user_path+"/"+n.second,"new_dir");
        }
        for (std::pair<std::string, std::string> n : fileNames) {
            if (n.first == "file")
                sendFile(client_socket, n.second, user_path + "/" + n.second, "new_file");
        }

    }


