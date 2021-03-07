//
// Created by matteo on 25/02/21.
//

#ifndef PROJE_HASHC_H
#define PROJE_HASHC_H
#include<iostream>
#include<stdlib.h>
#include "openssl/sha.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

#include <openssl/sha.h>
#include <boost/filesystem/path.hpp>


class HashC {
public:
    void sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65]);
    void sha256_string(char *string, char outputBuffer[65]);
    int sha256_file(char* path, char outputBuffer[65]);
    int hashingFile(boost::filesystem:: path p);
    std::string get_hash_file(boost::filesystem:: path p);

};


#endif //PROJE_HASHC_H
