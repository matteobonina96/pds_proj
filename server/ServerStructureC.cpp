//
// Created by matteo on 25/02/21.
//

#include "ServerStructureC.h"


void ServerStructureC::create_sync_structure_server() {
    if(boost::filesystem::exists("utility/server-struct.json"))
        boost::filesystem::remove("utility/server-struct.json");
    if(boost::filesystem::exists("utility/client-struct.json"))
        boost::filesystem::remove("utility/client-struct.json");
    json temp;
    temp["hashed_status"] = "empty_hashed_status";
    temp["entries"] = json::array();
    std::ofstream out{"utility/server-struct.json"};
    out << std::setw(4) << temp << std::endl;

}

//std::unique_ptr<json> structure = std::make_unique<json>();


void ServerStructureC::write_sync_structure_server(json s)  {
    create_sync_structure_server();
    std::ofstream out{"utility/server-struct.json"};
    out<< s<< "\n";
    out.close();
}

void ServerStructureC::read_structure_client(std::unordered_map<std::string, json> &entries) {
    std::ifstream i("utility/client-struct.json");
    std::unique_ptr<json> structure = std::make_unique<json>();
    i >> (*structure);
    //pulisco un po
    for (size_t i = 0; i < (*structure)["entries"].size(); i++) {
        std::string path = (*structure)["entries"][i]["path"];
        json entry = (*structure)["entries"][i];
        entries[path] = entry;
    }
    //cout<<std::endl<<std::endl<<entries;
    //cout<<std::endl<<entries["asdasd/ProvaDifest"]["hash"];

}
void ServerStructureC::read_structure_server(std::unordered_map<std::string, json> &entries_s) {
    std::ifstream i("utility/server-struct.json");
    std::unique_ptr<json> structure = std::make_unique<json>();
    i >> (*structure);
    //pulisco un po
    for (size_t i = 0; i < (*structure)["entries"].size(); i++) {
        std::string path = (*structure)["entries"][i]["path"];
        json entry = (*structure)["entries"][i];
        entries_s[path] = entry;
    }
    //cout<<std::endl<<std::endl<<entries;
    //cout<<std::endl<<entries["asdasd/ProvaDifest"]["hash"];

}

boost::property_tree::ptree ServerStructureC::confronto_sync_client_to_server(std::string path) {
    std::unordered_map<std::string, json> entries;
    std::unordered_map<std::string, json> entries_s;
    read_structure_client(entries);
    read_structure_server(entries_s);
    //cout<<std::endl<<entries<<std::endl;
    //cout<<std::endl<<entries_s<<std::endl;

    boost::property_tree::ptree root;
    boost::property_tree::ptree files;
    root.put("type","server_needs_for_sync");


    //cout<<std::endl<<"Confronto se quelli presenti nel client sono presenti nel server, con lo stesso hash:"<<std::endl;

    for( const auto& n : entries) {

        std::unordered_map<std::string,json>::const_iterator it = entries_s.find (n.first);
        if ( it == entries.end() ) {
            std::cout << n.first << " non trovato nel server! DEVO RICEVERLO DAL CLIENT!"<<std::endl;
            boost::property_tree::ptree file;
            file.put("", n.first);
            files.push_back(std::make_pair(n.second["type"], file));
        }
        else {
            std::cout << it->first << " trovato, ";
            if(it->second["hash"] == n.second["hash"])
                std::cout<<"hanno pure lo stesso hash!!!"<<std::endl;
            else {
                std::cout<<"NON HANNO lo stesso hash!!! DEVO RICEVERLO AGGIORNATO DAL CLIENT"<<std::endl;

                boost::property_tree::ptree file;
                file.put("", n.first);
                files.push_back(std::make_pair(n.second["type"], file));
                //rimuovo questo, ricevo quello nuovo
                boost::filesystem::remove_all(path+"/"+n.first);


            }
        }
    }

    cout<<std::endl<<"Confronto se quelli presenti nel server sono ancora presenti nel client, con lo stesso hash, per capire quali eliminare perche non piu presenti:"<<std::endl;

    for( const auto& n : entries_s) {
        //cout<<std::endl<<n.second["path"];
        //cout<<std::endl<<n.second["hash"];
        std::unordered_map<std::string,json>::const_iterator it = entries.find (n.first);
        cout<<std::endl;
        if ( it == entries_s.end() ) {
            std::cout << n.first << " DEVO ELIMINARLO PERCHE NON PIU NEL CLIENT" << std::endl;
            boost::filesystem::remove_all(path+"/"+n.first);
        } }

    root.add_child("fileNames", files);
    return root;


}


boost::property_tree::ptree ServerStructureC::confronto_sync_server_to_client(std::string path) {
    std::unordered_map<std::string, json> entries;
    std::unordered_map<std::string, json> entries_s;
    read_structure_client(entries);
    read_structure_server(entries_s);
    //cout<<std::endl<<entries<<std::endl;
    //cout<<std::endl<<entries_s<<std::endl;

    boost::property_tree::ptree root;
    boost::property_tree::ptree files;
    root.put("type","client_needs_for_sync");


    cout<<std::endl<<"Confronto se quelli presenti nel server sono presenti nel client, con lo stesso hash:"<<std::endl;
    for( const auto& n : entries_s) {

        std::unordered_map<std::string,json>::const_iterator it = entries.find (n.first);
        if ( it == entries_s.end() ) {
            std::cout << n.first<<" non trovato nel client! DEVO MANDARLO PERCHE' NON C'È"<<std::endl;
            boost::property_tree::ptree file;
            file.put("", n.first);
            files.push_back(std::make_pair(n.second["type"], file));
        }
        else {
            std::cout << it->first << " trovato, ";
            if(it->second["hash"] == n.second["hash"])
                std::cout<<"hanno pure lo stesso hash!!!"<<std::endl;
            else {
                std::cout<<"NON HANNO lo stesso hash!!! DEVO MANDARLO"<<std::endl;

                boost::property_tree::ptree file;
                file.put("", n.first);
                files.push_back(std::make_pair(n.second["type"], file));


            }
        }
    }


    root.add_child("fileNames", files);
    return root;


}


