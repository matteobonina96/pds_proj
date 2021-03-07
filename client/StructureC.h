//
// Created by matteo on 25/02/21.
//

#ifndef PROJE_STRUCTUREC_H
#define PROJE_STRUCTUREC_H
#include <iostream>
#include <vector>
#include <algorithm>
#define BOOST_NO_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_SCOPED_ENUMS

#include <chrono>
#include "../hash.h"
#include <vector>
#include <chrono>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <string>
#include <utility>
#include <condition_variable>
#include <iomanip>
#include "../utility/json.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using json = nlohmann::json;

using std::cout;
using namespace boost::filesystem;

class StructureC {
    //std::unique_ptr<json> structure = std::make_unique<json>();
public:
    void create_sync_structure();
    void write_sync_structure_client(json s);
    void write_sync_structure_server(json s);
    void read_structure_client(std::unordered_map<std::string, json> &entries);
    void read_structure_server(std::unordered_map<std::string, json> &entries_s);

    boost::property_tree::ptree confronto_sync_server_to_client(std::string path);

    boost::property_tree::ptree confronto_sync_client_to_server(std::string path);
};


#endif //PROJE_STRUCTUREC_H
