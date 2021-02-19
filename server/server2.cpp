#include <boost/asio.hpp>
#include <iostream>
#include <sqlite3.h>
#include <thread>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include "FileManage.cpp"
#include "ServerStructureManage.cpp"
#include "users.h"


using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
namespace pt = boost::property_tree;

using boost::asio::ip::tcp;
int noThread = 0;
char *zErrMsg = 0;
string id; //Stringa dove memorizzo l'id
string pass; //Stringa password
string up_select; //Stringa percorso cartella
string response, reply,richiesta_pass, password;
string scelta, sql;
int num_scelta;
int utente_autenticato;
sqlite3* DB; //dichiaro un database sqlite3
int res = 0; //variabile per il risultato dell'apertura del db

string menu1="Scegli una delle seguenti opzioni:\n1)Iscriviti\n2)Accedi";
string menu2="Scegli una delle seguenti opzioni:\n   1)Scarica sul server l'attuale configurazione\n    2)Effettua backup dal server nella tua cartella\nDopo di ciò, l'applicazione funzionerà in background come filewatcher.\n";


void sendData(tcp::socket& socket, const string& message);
string getData(tcp::socket& socket);
std::string iscrizione(tcp::socket& server_socket);
std::string login(tcp::socket& server_socket);
void sync_client_to_server(tcp::socket& server_socket,std::string dir);
void sync_server_to_client(tcp::socket& server_socket,std::string dir);
void sendFileForSyncro(tcp::socket& server_socket, std::string user_path);
void receiveFileForSyncro(tcp::socket& server_socket, std::string user_path);



//SQLCALLBACK - funzione che mi restituisce i valori della SELECT
static int callback(void* data, int argc, char** argv, char** azColName)
{
    cout<<"OMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM";
    int i;
    fprintf(stderr, "%s", (const char*)data);

    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    id=argv[0];

    pass=argv[1];
    up_select=argv[2];
    printf("\n");
    return 0;
}

std::string iscrizione(tcp::socket& server_socket) {
    //autenticazione iniziale
    users user;
    string response;
    sendData(server_socket,"Username:");
    response = getData(server_socket);
    response.pop_back();
    user.setUsername(response);
    sendData(server_socket,"Password");
    response = getData(server_socket);
    response.pop_back();
    user.setPassword(response);
    std::string dir = "ServerFS"+user.getUsername();
    user.setUserPath(dir);
    cout<<std::endl<<std::endl<<user.getUserPath();

    //apro il db
    std::lock_guard<std::mutex> lg(mutex_structure);

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


std::string login(tcp::socket& server_socket) {
    users user;
    string response;
    sendData(server_socket,"Username:");
    response = getData(server_socket);
    response.pop_back();
    user.setUsername(response);
    sendData(server_socket,"Password");
    response = getData(server_socket);
    response.pop_back();
    user.setPassword(response);


    //apro il db
    std::lock_guard<std::mutex> lg(mutex_structure);
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
        cout << "Dati trovati. " << endl << endl;
    }
    sqlite3_close(DB);

    if(up_select == "\0" ) //vuol dire che non ha trovato corrispondenze in tabella, ergo l'utente non esiste
    {
        cout<<"#####################ERROR###################"<<std::endl<<"Utente non trovato";
        return "-1";
    }
    return up_select;

}

void start_server_config(boost::filesystem::path p) {
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
                    row["hash"] = get_hash_file(it->path());
                    row["timestamp_last_mod"] = boost::filesystem::last_write_time(it->path());

                    structure["entries"].push_back(row);
                    v.push_back(it->path());
                }
                write_sync_structure_server(structure);
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
void sendData(tcp::socket& socket, const string& message)
{
    boost::asio::write(socket,
          buffer(message + "\n"));
}

string getData(tcp::socket& socket)
{
    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\n");
    string data = buffer_cast<const char*>(buf.data());
    cout<<"Received<<"<<std::endl<<data;
    return data;
}

int getData2(tcp::socket& socket,std::string user_path) {
    std::array<char, 512> resp;
    memset(&resp,0,resp.size());
    boost::asio::read(socket, buffer(resp,512));
    std::string s(resp.data());
    std::stringstream ss(s);
    pt::ptree root;
    std::cout <<"JSON from client: "<<std::endl<<s<< std::endl;
    boost::property_tree::json_parser::read_json(ss, root);
    std::string type = root.get<std::string>("type");
    std::string mod = root.get<std::string>("how_to");
    if(type=="logout") {
        //DEVO SLOGGARE E TERMINARE LA CONNESSIONE
        cout<<"Devo sloggare";
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

        readFile(socket,user_path,path, size); }
    }
} }



void getDataForServerToClientSync(tcp::socket& socket,std::string user_path) {
    std::array<char, 512> resp;
    memset(&resp,0,resp.size());
    boost::asio::read(socket, buffer(resp,512));
    std::string s(resp.data());
    std::stringstream ss(s);
    boost::property_tree::ptree root;
    std::cout <<"JSON from client: "<<std::endl<<s<< std::endl;
    boost::property_tree::json_parser::read_json(ss, root);

    std::string type = root.get<std::string>("type");

    if (type == "directory") { // nel json ho ricevuto info su una directory
        createDirectory(root.get<std::string>("path"),user_path+"/"+root.get<std::string>("path"));

    } else if (type=="file" ){ // nel json ho ricevuto info su un file

        std::string path = root.get<std::string>("path");
        size_t size = (root.get<size_t>("size"));
        readFile(socket,user_path, path, size);
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
                sendDir(socket,n.second,user_path+"/"+n.second,"new_dir");
        }
        for (std::pair<std::string, std::string> n : fileNames) {
            if (n.first == "file")
                sendFile(socket, n.second, user_path + "/" + n.second, "new_file");
        }

    }
}



void session(tcp::socket server_socket)
{
    std::string dir="-1"; //inizializzo a -1 per il while
    std::string scelta;
    std::string resp;
    cout << "CLIENT thread n° " << this_thread::get_id() << " opened." << endl;
    try {
        //first chat -> menu1
        while(1) {
            sendData(server_socket,menu1);
            resp = getData(server_socket);
            // Popping last character "\n"
            resp.pop_back();
            if(resp=="1" || resp=="2") break;
            }

        //authentication
        while(dir=="-1") {
            if (resp == "1") dir = iscrizione(server_socket);
            if (resp == "2") dir = login(server_socket);

        }

        //Scelta da menu2 per tipologia di backup
        while(1) {
            sendData(server_socket,menu2);
            resp = getData(server_socket);
            // Popping last character "\n"
            resp.pop_back();
            scelta=resp;
            if(scelta=="1" || scelta=="2") break;
        }

        //send comando start_config
            sendData(server_socket,"start_config_req");
            resp = getData(server_socket);
            // Popping last character "\n"
            resp.pop_back();
            if(resp=="start_config_ok")
            sendData(server_socket,scelta);
            else { //error
                 }

            if (scelta=="1") sync_client_to_server(server_socket,dir); //classic
            if (scelta=="2") sync_server_to_client(server_socket,dir);

            //fileWatcher
            while(1)  //attesa filewatcher
                getData2(server_socket,dir);


    }
    catch (std::exception &e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }}

void sync_client_to_server(tcp::socket& server_socket,std::string dir) {
        std::lock_guard<std::mutex> lg(mutex_structure);

        //creo la struct per la sync
            start_server_config(dir);


        //mando server struct
        sendFile(server_socket,"server-struct.json","utility/server-struct.json","start_config");
        // prendo client struct
        getData2(server_socket,dir);

        //ricevo Files per sincronizazzione
        receiveFileForSyncro(server_socket,dir);

}


void sync_server_to_client(tcp::socket& server_socket,std::string dir) {
    std::lock_guard<std::mutex> lg(mutex_structure);
        //creo la struct per la sync
        start_server_config(dir);

        //prendo client-struct
        getData2(server_socket,dir);


        //mando server-struct
        sendFile( server_socket,"server-struct.json","utility/server-struct.json","start_config");

        //calcolo quello che devo mandare al client(avendo le 2 struct) e mando
        sendFileForSyncro(server_socket,dir);

}



void sendFileForSyncro(tcp::socket& client_socket, std::string user_path) {
    //in root ottengo un tree con il nome dei file da mandare al client
    boost::property_tree::ptree root = confronto_sync_server_to_client(user_path);

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

void receiveFileForSyncro(tcp::socket& server_socket, std::string dir) {
    boost::property_tree::ptree root = confronto_sync_client_to_server(dir);

    for(int j=0;j<root.get_child("fileNames").size();j++) {
        getData2(server_socket,dir);
    }

}



void server(boost::asio::io_service& io_service)
{
    // Listening for any new incomming connection
    // at port 9999 with IPv4 protocol
    tcp::acceptor acceptor_server(
            io_service,
            tcp::endpoint(tcp::v4(), 9999));

    for (;;)
    {
        // Creating socket object
        tcp::socket server_socket(io_service);
        // waiting for connection
        acceptor_server.accept(server_socket);
        cout << endl << "Socket opened and accepted." << endl;
        //opening a thread for the current client
        std::thread(session, std::move(server_socket)).detach();
        noThread++;
        cout << "noThread: " << noThread << endl;

    }

}

int main(int argc, char* argv[])
{



    io_service io_service;

    server(io_service);

    return 0;
}


