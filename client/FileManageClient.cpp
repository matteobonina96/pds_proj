
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>



using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;


void createDirectory(std::string path) {
    boost::filesystem::create_directories(path);
}
void deleteSomething(std::string path) {
    boost::filesystem::remove(path);
}
void createFile(std::string user_path,std::string name, std::string text) {
    cout<<"I'm creating file "<<name<<std::endl;
    boost::filesystem::ofstream ofs;
    if(name=="server-struct.json") {
        ofs.open("utility/"+name, std::ios::app);
    }
    else
        ofs.open(user_path+"/"+name, std::ios::app);
    ofs << std::flush << text;
}

boost::property_tree::ptree createPtreeDirectory(std::string path,std::string how_to) {
    boost::property_tree::ptree root;
    root.put("type", "directory");
    root.put("path", path);
    root.put("how_to",how_to);

    return root;
}


boost::property_tree::ptree createPtreeFile(std::string path,size_t size,std::string how_to) {
    boost::property_tree::ptree root;
    root.put("type", "file");
    root.put("path", path);
    root.put("size", size);
    root.put("how_to",how_to);
    return root;
}


void sendDir(tcp::socket& socket,std::string filename,std::string path,std::string mod) {
    boost::property_tree::ptree root = createPtreeDirectory(filename,mod);
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout << "JSON to server : " << std::endl << ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json della directory
    s.length() < 512 ? s.append(" ", 512 - s.length()) : NULL;
    boost::asio::write(socket, buffer(s, 512));

}

void sendFile(tcp::socket& socket,std::string filename,std::string path,std::string mod) {
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
    std::cout << "JSON to server : " << std::endl <<  ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json del file
    s.length() < 512 ? s.append(" ",512-s.length()) : NULL;
    boost::asio::write(socket, buffer(s, 512));
    if(mod!="file_to_delete") {
        // INVIO DEL CONTENUTO (se invio file_to_Delete mando solo il json con il nome, quindi mi fermo prima
        unsigned int writeCounter = fileSize;
        int readFileCounter = 0;
        std::array<char,512> bufferFile;
        while(writeCounter > 0 && readFileCounter< fileSize){
            memset(&bufferFile,0,bufferFile.size());
            file.read(bufferFile.data(),writeCounter > 512 ? 512 : writeCounter);
            readFileCounter += file.gcount();
            file.seekg(readFileCounter);
            writeCounter -= boost::asio::write(socket,buffer(bufferFile,writeCounter > 512 ? 512 : writeCounter));
        }
        file.close(); }

}

void sendLogOut(tcp::socket& socket) {

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
    s.length() < 512 ? s.append(" ",512-s.length()) : NULL;
    boost::asio::write(socket, buffer(s, 512));


}


void readFile(tcp::socket& socket,std::string user_path, std::string path, int size){
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
            cout<<"ReadCounter ->"<<std::endl<<readCounter;
            std::array<char, 512> response;
            memset(&response,0,response.size());
            c = boost::asio::read(socket,
                                  boost::asio::buffer(response, readCounter > 512 ? 512 : readCounter ));
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
}


int sendFileNamesClientNeeds(tcp::socket& socket,boost::property_tree::ptree root) {
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout <<std::endl<< "_My New JSON of What Client Needs : " << std::endl <<  ss.str() << std::endl;
    std::cout << "JSON to server : " << std::endl <<  ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json del file
    s.length() < 512 ? s.append(" ",512-s.length()) : NULL;
    boost::asio::write(socket, buffer(s, 512));

}



