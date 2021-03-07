//
// Created by matteo on 25/02/21.
//

#include "ClientC.h"


void ClientC::sendData_string(const std::string& message)
{
    write(client_socket,
          buffer(message + "\n"));
}

std::string ClientC::getData_string() {
    boost::asio::streambuf buf;
    boost::asio::read_until(client_socket, buf, "\n");
    std::string data = buffer_cast<const char*>(buf.data());
    return data;
}

void ClientC::getData() {
    std::array<char, SIZE_BLOCK> resp;
    memset(&resp,0,resp.size());
    boost::asio::read(client_socket, buffer(resp,SIZE_BLOCK));

    std::string s(resp.data());
    std::stringstream ss(s);
    boost::property_tree::ptree root;
    std::cout <<"From server to Client - JSON: "<<std::endl<<s<< std::endl;
    boost::property_tree::json_parser::read_json(ss, root);

    std::string type = root.get<std::string>("type");

    if (type == "directory") { // nel json ho ricevuto info su una directory
        createDirectory(user_path+"/"+root.get<std::string>("path"));

    } else if (type=="file" ){ // nel json ho ricevuto info su un file

        std::string path = root.get<std::string>("path");
        size_t size = (root.get<size_t>("size"));
        readFile(path, size);
    }
}



void ClientC::sendLogOut() {

//costruisco il json DI LOGOUT

    boost::property_tree::ptree root;
    root.put("type", "logout");
    root.put("path", "logout");
    root.put("size", "logout");
    root.put("how_to","logout");
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout << "JSON to server : " << std::endl <<  ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json
    s.length() < SIZE_BLOCK ? s.append(" ",SIZE_BLOCK-s.length()) : NULL;
    boost::asio::write(client_socket, buffer(s, SIZE_BLOCK));


}


void ClientC::sync_operation(boost::filesystem::path p) {

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
                        row["hash"] = hc.get_hash_file(it->path());
                    row["timestamp_last_mod"] = boost::filesystem::last_write_time(it->path());

                    structure["entries"].push_back(row);
                    v.push_back(it->path());
                }
                sc.write_sync_structure_client(structure);
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

void ClientC::fileW() {
    int base = user_path.length();
    FileWatcher fw{user_path,base,std::chrono::milliseconds(5000)};

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([this] (std::string path_to_watch,int base, FileStatus status) -> void {
        // Process only regular files and directory, all other file types are ignored
        if(!(boost::filesystem::is_regular_file(boost::filesystem::path(path_to_watch)) || boost::filesystem::is_directory(boost::filesystem::path(path_to_watch))) && status != FileStatus::erased) {
            return;
        }
        int y;
        switch(status) {
            case FileStatus::directory_created:
                std::cout<< "New directory created :"<< path_to_watch<<std::endl;
                y = path_to_watch.length();
                sendDir(path_to_watch.substr(base+1,y),path_to_watch,"new_dir");
                break;
            case FileStatus::created:
                std::cout << "File created: " << path_to_watch << '\n';
                y = path_to_watch.length();
                sendFile(path_to_watch.substr(base+1,y),path_to_watch,"new_file");
                break;
            case FileStatus::modified:
                std::cout << "File modified: " << path_to_watch << '\n';
                y = path_to_watch.length();
                sendFile(path_to_watch.substr(base+1,y),path_to_watch,"updated_file");
                break;
            case FileStatus::erased:
                std::cout << "File erased: " << path_to_watch << '\n';
                y = path_to_watch.length();
                sendFile(path_to_watch.substr(base+1,y),path_to_watch,"file_to_delete");
                break;

            default:
                std::cout << "Error! Unknown file status.\n";
        }
    });

}

void ClientC::sync_server_to_client() {
    sync_operation(user_path);
    //mando client struct
    sendFile("client-struct.json","utility/client-struct.json","start_config");

    // ricevo server-struct
    getData();

    receiveFileForSyncro();
    cout<<"Sincronizzazione terminata."<<std::endl;

}

void ClientC::sync_client_to_server() {
    //Creo le struct per la sincronizazzione
    sync_operation(user_path);

    //ricevo server-struct
    getData();

    //invio client-struct
    sendFile( "client-struct.json","utility/client-struct.json","start_config");

    //Mando i file al server
    sendFileForSyncro();
    cout<<"Sincronizzazione terminata."<<std::endl;


}

void ClientC::receiveFileForSyncro() {
    boost::property_tree::ptree root = sc.confronto_sync_server_to_client(user_path);
    cout<<"Devo ricevere ->"<<root.get_child("fileNames").size()<< " elementi"<<std::endl;
    for(int j=0;j<root.get_child("fileNames").size();j++) {
        getData();
    }
}

void ClientC::sendFileForSyncro() {
    //Ottengo un tree con i file da inviare, ottenuto da confronto tra server/client struct
    boost::property_tree::ptree root = sc.confronto_sync_client_to_server(user_path);

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
            sendDir(n.second,user_path+"/"+n.second,"new_dir");
    }
    for (std::pair<std::string, std::string> n : fileNames) {
        if (n.first == "file")
            sendFile(n.second, user_path + "/" + n.second, "new_file");
    }

}


void ClientC::logout(tcp::socket&& socket) {
    std::string reply;
    try {
        std::cout << "Digita il comando EXIT per uscire."<< std::endl;
        do {
            std::getline(std::cin, reply);
            std::cout<<reply<<std::endl;
            if (reply!= "EXIT") {
                std::cout << "Devi digitare EXIT!" << std::endl;
                std::cin.clear();
            }

        }while (reply!="EXIT");
        sendLogOut();
        socket.close();
        std::exit(EXIT_SUCCESS);

    }catch(std::exception &ex){
        //
    }
}


void ClientC::handle_connection() {

    std::string response,scelta_menu;
    while (true) {
        //SCELTA
        response = getData_string();
        response.pop_back();
        cout << "Server: " << response << std::endl;
        if(response=="start_config_req")  {
            sendData_string("start_config_ok");
            break;
        }
        //mando
        getline(std::cin, scelta_menu);
        sendData_string(scelta_menu);

    }


    if (scelta_menu=="1") sync_client_to_server();
    if (scelta_menu=="2") sync_server_to_client();



    std::thread t_logout([this](){
        logout(std::move(client_socket));
    });

    t_logout.detach();
    cout<<"********** FileWatcher in background **********"<<std::endl;
    //lancio file watcher in background
    fileW();


}


void ClientC::createDirectory(std::string path) {
    boost::filesystem::create_directories(path);
}
void ClientC::deleteSomething(std::string path) {
    boost::filesystem::remove(path);
}
void ClientC::createFile(std::string name, std::string text) {
    cout<<"I'm creating file "<<name<<std::endl;
    boost::filesystem::ofstream ofs;
    if(name=="server-struct.json") {
        ofs.open("utility/"+name, std::ios::app);
    }
    else
        ofs.open(user_path+"/"+name, std::ios::app);
    ofs << std::flush << text;
}

boost::property_tree::ptree ClientC::createPtreeDirectory(std::string path,std::string how_to) {
    boost::property_tree::ptree root;
    root.put("type", "directory");
    root.put("path", path);
    root.put("how_to",how_to);

    return root;
}


boost::property_tree::ptree ClientC::createPtreeFile(std::string path,size_t size,std::string how_to) {
    boost::property_tree::ptree root;
    root.put("type", "file");
    root.put("path", path);
    root.put("size", size);
    root.put("how_to",how_to);
    return root;
}


void ClientC::sendDir(std::string filename,std::string path,std::string mod) {
    boost::property_tree::ptree root = createPtreeDirectory(filename,mod);
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout << "From client to Server - JSON: " << std::endl << ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json della directory
    s.length() < SIZE_BLOCK ? s.append(" ", SIZE_BLOCK - s.length()) : NULL;
    boost::asio::write(client_socket, buffer(s, SIZE_BLOCK));

}

void ClientC::sendFile(std::string filename,std::string path,std::string mod) {
    // open the file:
    std::ifstream file(path, std::ifstream::binary);
    file.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

//costruisco il json di un file
    boost::property_tree::ptree root = createPtreeFile(filename,fileSize,mod);
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout << "From client to Server - JSON: " << std::endl << ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json del file
    s.length() < SIZE_BLOCK ? s.append(" ",SIZE_BLOCK-s.length()) : NULL;
    boost::asio::write(client_socket, buffer(s, SIZE_BLOCK));
    if(mod!="file_to_delete") {
        // INVIO DEL CONTENUTO (se invio file_to_Delete mando solo il json con il nome, quindi mi fermo prima
        unsigned int writeCounter = fileSize;
        int readFileCounter = 0;
        std::array<char,SIZE_BLOCK> bufferFile;
        while(writeCounter > 0 && readFileCounter< fileSize){
            memset(&bufferFile,0,bufferFile.size());
            file.read(bufferFile.data(),writeCounter > SIZE_BLOCK ? SIZE_BLOCK : writeCounter);
            readFileCounter += file.gcount();
            file.seekg(readFileCounter);
            writeCounter -= boost::asio::write(client_socket,buffer(bufferFile,writeCounter > SIZE_BLOCK ? SIZE_BLOCK : writeCounter));
        }
        file.close(); }

}


void ClientC::readFile(std::string path, int size){
    int readCounter=size;
    int c = 0;
    std::cout<<"I'm in readFile'"<<std::endl;
    if(readCounter == 0){
        std::cout << "file con dimensione 0"<< std::endl;
        boost::filesystem::ofstream ofs;
        ofs.open(path, std::ios::app);
    }
    try{
        while(readCounter > 0){
            std::array<char, SIZE_BLOCK> response;
            memset(&response,0,response.size());
            c = boost::asio::read(client_socket,
                                  boost::asio::buffer(response, readCounter > SIZE_BLOCK ? SIZE_BLOCK : readCounter ));
            readCounter -= c;
            std::string text{response.begin(), response.begin() + c};
            createFile(path, text );
        }
    }catch(boost::system::system_error e) {
        if(e.code() == boost::asio::error::eof) {
            if(readCounter > 0) {
                std::cout<<"IL FILE E' CORROTTO"<<std::endl;
                deleteSomething(path);
            }
            throw boost::system::system_error(boost::asio::error::eof);       //utilizzare questa nei "figli"
        }
    }
}




