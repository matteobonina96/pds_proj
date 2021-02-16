
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

void createDirectory(std::string path,std::string user_path) {
    boost::filesystem::create_directories(user_path+"/"+path);
}

void createNewDir(std::string path) {
    boost::filesystem::create_directories(path);
}
void deleteSomething(std::string path) {
    boost::filesystem::remove(path);
}
void createFile(std::string user_path,std::string path, std::string text) {
    boost::filesystem::ofstream ofs;
    if(path=="client-struct.json") {
        ofs.open("utility/"+path, std::ios::app);
    }
    else
        ofs.open(user_path+"/"+path, std::ios::app);
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
    std::cout << "JSON to client : " << std::endl <<  ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json del file
    s.length() < 512 ? s.append(" ",512-s.length()) : NULL;
    boost::asio::write(socket, buffer(s, 512));
    // INVIO DEL CONTENUTO
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
    file.close();

}

void sendDir(tcp::socket& socket,std::string filename,std::string path,std::string mod) {
    boost::property_tree::ptree root = createPtreeDirectory(filename,mod);
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout << "JSON to client : " << std::endl << ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json della directory
    s.length() < 512 ? s.append(" ", 512 - s.length()) : NULL;
    boost::asio::write(socket, buffer(s, 512));

}

void readFile(tcp::socket& socket,std::string user_path, std::string path, int size){
    int readCounter=size;
    int c = 0;
    std::cout<<"IM IN READ FILE"<<std::endl;
    if(readCounter == 0){
        std::cout << "file con dimensione 0"<< std::endl;
        boost::filesystem::ofstream ofs;
        ofs.open(path, std::ios::app);
    }
    try{
        while(readCounter > 0){
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
    std::cout<<std::endl;
}

void sendFileNamesServerNeeds(tcp::socket& socket,boost::property_tree::ptree root) {
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    std::cout <<std::endl<< "____________My New JSON of What Server Needs : " << std::endl <<  ss.str() << std::endl;
    std::cout << "JSON to client : " << std::endl <<  ss.str() << std::endl;
    std::string s{ss.str()};
    // invio il json del file poco alla volta, nel caso in cui ci siano tantissimi file
    cout<<"S length() : "<<std::endl<<s.length()<<std::endl;
    /*
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

    */


        //s.length() < 512 ? s.append(" ",512-s.length()) : NULL;
        //boost::asio::write(socket, buffer(s, 512));
        std::string mess="funzia";
    boost::asio::write(socket,buffer(mess + "\n"));
    }




//}
