//
// Created by matteo on 25/02/21.
//

#include "ServerC.h"


std::string user_path_from_select;

void ServerC::logout(tcp::socket&& socket) {
    socket.close();
}
//SQLCALLBACK - funzione che mi restituisce i valori della SELECT
static int callback(void* data, int argc, char** argv, char** azColName)
{
    int i;
    fprintf(stderr, "%s", (const char*)data);

    for (i = 0; i < argc; i++) {
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    user_path_from_select=argv[2];
    cout<<"Sto in callback: "<<user_path_from_select<<std::endl;

    return 0;
}

std::string ServerC::iscrizione() {

    //autenticazione iniziale
    users user;
    string response;
    int res;
    sendData("Username:");
    response = getData();
    response.pop_back();
    user.setUsername(response);
    sendData("Password");
    response = getData();
    response.pop_back();
    user.setPassword(response);
    std::string dir = "ServerFS"+user.getUsername();
    user.setUserPath(dir);
    cout<<std::endl<<std::endl<<user.getUserPath();

    //apro il db
    std::lock_guard<std::mutex> lg(mutex_db);

    res = sqlite3_open("test.db", &DB);
    string sql("INSERT into users ('ID', 'PASS', 'PATH') values ('"+user.getUsername()+"','"+user.getPassword()+"','"+user.getUserPath()+"')");

    if (res) {
        std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
        return "-1";
    } else
        std::cout << "Database aperto con successo!" << std::endl;

    int rc = sqlite3_exec(DB, sql.c_str(), callback, 0, NULL);
    if (rc != SQLITE_OK) {
        cerr << "Error INSERT" << endl;
        return "-1"; }
    else {

        cout << "Dati inseriti. Adesso puoi accedere" << endl << endl;
    }
    sqlite3_close(DB);

    //creo una directory per l'utente
    createNewDir(dir);
    return dir;

}


std::string ServerC::login() {
    users user;
    string response;
    int res;
    sendData("Username:");
    response = getData();
    response.pop_back();
    user.setUsername(response);
    sendData("Password");
    response = getData();
    response.pop_back();
    user.setPassword(response);



    //apro il db
    std::lock_guard<std::mutex> lg(mutex_db);

    user_path_from_select = "\0";
    up_select = "\0"; //QUANDO FACCIO LOGOUT DEVO PULIRE QUESTA STRINGA PER IL SUCCESSIVO LOGIN

    res = sqlite3_open("test.db", &DB);
    string data("Ricerca nel database...\n");
    string sql("select * from users where ID = '"+user.getUsername()+"' and PASS = '"+user.getPassword()+"';  ");


    if (res) {
        std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
        return "-1";
    } else
        std::cout << "Database aperto con successo!" << std::endl;

    int rc = sqlite3_exec(DB, sql.c_str(), callback, (void *) data.c_str(), NULL);
    if (rc != SQLITE_OK) {
        cerr << "Error select" << endl;
        return "-1"; }
    else {

        up_select=user_path_from_select;
        cout<<"user_path_from_select: " <<user_path_from_select<<std::endl;
        if(up_select == "\0" ) //vuol dire che non ha trovato corrispondenze in tabella, ergo l'utente non esiste
        {
            cout<<"#####################ERROR###################"<<std::endl<<"Utente non trovato";
            return "-1";
        }
        cout << "Dati trovati. " << endl << endl;
    }
    sqlite3_close(DB);


    return up_select;

}

void ServerC::start_server_config(boost::filesystem::path p) {
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
                std::vector<boost::filesystem::path> v;
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
                sc.write_sync_structure_server(structure);
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
    }}


// Funzione per invio di dati
void ServerC::sendData( const string& message)
{
    boost::asio::write(server_socket,
                       buffer(message + "\n"));
}

string ServerC::getData()
{
    boost::asio::streambuf buf;
    boost::asio::read_until(server_socket, buf, "\n");
    string data = buffer_cast<const char*>(buf.data());
    cout<<"Received ->"<<data<<std::endl;
    return data;
}

int ServerC::getData2(std::string user_path) {
    std::array<char, SIZE_BLOCK> resp;
    memset(&resp,0,resp.size());
    boost::asio::read(server_socket, buffer(resp,SIZE_BLOCK));
    std::string s(resp.data());
    std::stringstream ss(s);
    pt::ptree root;
    std::cout << "From client to Server - JSON: " << std::endl << ss.str() << std::endl;
    boost::property_tree::json_parser::read_json(ss, root);
    std::string type = root.get<std::string>("type");
    std::string mod = root.get<std::string>("how_to");
    if(type=="logout") {
        //DEVO SLOGGARE E TERMINARE LA CONNESSIONE
        server_socket.close();
        user_path_from_select = "\0";
        up_select = "\0"; //QUANDO FACCIO LOGOUT DEVO PULIRE QUESTA STRINGA PER IL SUCCESSIVO LOGIN
        return -2;
    } else {

        if (type == "directory") { // nel json ho ricevuto info su una directory
            createDirectory(root.get<std::string>("path"),user_path);
        } else if(type=="file") { // nel json ho ricevuto info su un file
            std::string path = root.get<std::string>("path");

            if(mod=="file_to_delete") {
                cout<<"I'm trying to remove "<<user_path+"/"+path<<std::endl;

                boost::filesystem::remove_all(user_path+"/"+path);

            }
            else {
                size_t size = (root.get<size_t>("size"));
                if(mod=="updated_file") {
                    cout<<"I'm trying to remove "<<user_path+"/"+path<<std::endl;
                    boost::filesystem::remove_all(user_path+"/"+path);
                }

                readFile(user_path,path, size); }
        }
    }
return 1;}



void ServerC::getDataForServerToClientSync(std::string user_path) {
    std::array<char, SIZE_BLOCK> resp;
    memset(&resp,0,resp.size());
    boost::asio::read(server_socket, buffer(resp,SIZE_BLOCK));
    std::string s(resp.data());
    std::stringstream ss(s);
    boost::property_tree::ptree root;
    std::cout << "From client to Server - JSON: " << std::endl << ss.str() << std::endl;
    boost::property_tree::json_parser::read_json(ss, root);

    std::string type = root.get<std::string>("type");

    if (type == "directory") { // nel json ho ricevuto info su una directory
        createDirectory(root.get<std::string>("path"),user_path+"/"+root.get<std::string>("path"));

    } else if (type=="file" ){ // nel json ho ricevuto info su un file

        std::string path = root.get<std::string>("path");
        size_t size = (root.get<size_t>("size"));
        readFile(user_path, path, size);
    }
    else if(type=="client_needs_for_sync") {
        //devo mandare una serie di file al server
        std::vector<std::pair<std::string, std::string>> fileNames;
        // Iterator over all fileNames
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
                sendFile( n.second, user_path + "/" + n.second, "new_file");
        }

    }
}



void ServerC::start(){
    string menu1="Scegli una delle seguenti opzioni:\n1)Iscriviti\n2)Accedi";
    string menu2="Scegli una delle seguenti opzioni:\n   1)Scarica sul server l'attuale configurazione\n    2)Effettua backup dal server nella tua cartella\nDopo di ciò, l'applicazione funzionerà in background come filewatcher.\n";

    std::thread t_sync;
    std::string dir="-1"; //inizializzo a -1 per il while
    std::string scelta;
    std::string resp;
    cout << "CLIENT thread n° " << this_thread::get_id() << " opened." << endl;
    try {
        //first chat -> menu1
        while(1) {
            sendData(menu1);
            resp = getData();
            // Popping last character "\n"
            resp.pop_back();
            if(resp=="1" || resp=="2") break;
        }

        //authentication
        while(dir=="-1") {
            if (resp == "1") dir = iscrizione();
            if (resp == "2") dir = login();

        }

        //Scelta da menu2 per tipologia di backup
        while(1) {
            sendData(menu2);
            resp = getData();
            // Popping last character "\n"
            resp.pop_back();
            scelta=resp;
            if(scelta=="1" || scelta=="2") break;
        }

        //send comando start_config
        sendData("start_config_req");
        resp = getData();
        // Popping last character "\n"
        resp.pop_back();
        if(resp=="start_config_ok")
            sendData(scelta);
        else { //error
        }

        if (scelta=="1") sync_client_to_server(dir); //classic
        if (scelta=="2") sync_server_to_client(dir);

        //fileWatcher
        while(1) { //attesa filewatcher
            if (getData2(dir) == -2) //exit
            {
                return;
            }
        }

    }
    catch (std::exception &e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }}

void ServerC::sync_client_to_server(std::string dir) {

    //creo la struct per la sync
    start_server_config(dir);


    //mando server struct
    sendFile("server-struct.json","utility/server-struct.json","start_config");
    // prendo client struct
    getData2(dir);

    //ricevo Files per sincronizazzione
    receiveFileForSyncro(dir);

}


void ServerC::sync_server_to_client(std::string dir) {
    //std::lock_guard<std::mutex> lg(mutex_structure);
    //creo la struct per la sync
    start_server_config(dir);

    //prendo client-struct
    getData2(dir);


    //mando server-struct
    sendFile( "server-struct.json","utility/server-struct.json","start_config");

    //calcolo quello che devo mandare al client(avendo le 2 struct) e mando
    sendFileForSyncro(dir);

}



void ServerC::sendFileForSyncro(std::string user_path) {
    //in root ottengo un tree con il nome dei file da mandare al client
    boost::property_tree::ptree root = sc.confronto_sync_server_to_client(user_path);

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
            sendFile( n.second, user_path + "/" + n.second, "new_file");
    }

}

void ServerC::receiveFileForSyncro(std::string dir) {
    boost::property_tree::ptree root = sc.confronto_sync_client_to_server(dir);

    for(int j=0;j<root.get_child("fileNames").size();j++) {
        getData2(dir);
    }

}

void ServerC::createDirectory(std::string path,std::string user_path) {
    boost::filesystem::create_directories(user_path+"/"+path);
}

void ServerC::createNewDir(std::string path) {
    boost::filesystem::create_directories(path);
}
void ServerC::deleteSomething(std::string path) {
    boost::filesystem::remove(path);
}
void ServerC::createFile(std::string user_path,std::string path, std::string text) {
    boost::filesystem::ofstream ofs;
    if(path=="client-struct.json") {
        ofs.open("utility/"+path, std::ios::app);
    }
    else
        ofs.open(user_path+"/"+path, std::ios::app);
    ofs << std::flush << text;
}


boost::property_tree::ptree ServerC::createPtreeDirectory(std::string path,std::string how_to) {
    boost::property_tree::ptree root;
    root.put("type", "directory");
    root.put("path", path);
    root.put("how_to",how_to);

    return root;
}

boost::property_tree::ptree ServerC::createPtreeFile(std::string path,size_t size,std::string how_to) {
    boost::property_tree::ptree root;
    root.put("type", "file");
    root.put("path", path);
    root.put("size", size);
    root.put("how_to",how_to);
    return root;
}
void ServerC::sendFile(std::string filename,std::string path,std::string mod) {
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
    std::cout << "From server to Client - JSON: " << std::endl << ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json del file
    s.length() < SIZE_BLOCK ? s.append(" ",SIZE_BLOCK-s.length()) : NULL;
    boost::asio::write(server_socket, buffer(s, SIZE_BLOCK));
    // INVIO DEL CONTENUTO
    unsigned int writeCounter = fileSize;
    int readFileCounter = 0;
    std::array<char,SIZE_BLOCK> bufferFile;
    while(writeCounter > 0 && readFileCounter< fileSize){
        memset(&bufferFile,0,bufferFile.size());
        file.read(bufferFile.data(),writeCounter > SIZE_BLOCK ? SIZE_BLOCK : writeCounter);
        readFileCounter += file.gcount();
        file.seekg(readFileCounter);
        writeCounter -= boost::asio::write(server_socket,buffer(bufferFile,writeCounter > SIZE_BLOCK ? SIZE_BLOCK : writeCounter));
    }
    file.close();

}

void ServerC::sendDir(std::string filename,std::string path,std::string mod) {
    boost::property_tree::ptree root = createPtreeDirectory(filename,mod);
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout << "From server to Client - JSON: " << std::endl << ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json della directory
    s.length() < SIZE_BLOCK ? s.append(" ", SIZE_BLOCK - s.length()) : NULL;
    boost::asio::write(server_socket, buffer(s, SIZE_BLOCK));

}

void ServerC::readFile(std::string user_path, std::string path, int size){
    int readCounter=size;
    int c = 0;
    std::cout<<"I'm in readFile"<<std::endl;
    if(readCounter == 0){
        std::cout << "file con dimensione 0"<< std::endl;
        boost::filesystem::ofstream ofs;
        ofs.open(path, std::ios::app);
    }
    try{
        while(readCounter > 0){
            std::array<char, SIZE_BLOCK> response;
            memset(&response,0,response.size());
            c = boost::asio::read(server_socket,
                                  boost::asio::buffer(response, readCounter > SIZE_BLOCK ? SIZE_BLOCK : readCounter ));
            readCounter -= c;
            std::string text{response.begin(), response.begin() + c};
            createFile(user_path,path, text );
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
    std::cout<<std::endl;
}






