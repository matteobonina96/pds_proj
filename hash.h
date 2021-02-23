//
// Created by matteo on 13/11/20.
//
#pragma once
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

void sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
    int i = 0;

    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }

    outputBuffer[64] = 0;
}


void sha256_string(char *string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

int sha256_file(char* path, char outputBuffer[65])
{
    FILE *file = fopen(path, "rb");
    if(!file) return -534;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char *buffer = static_cast<unsigned char *>(malloc(bufSize));
    int bytesRead = 0;
    if(!buffer) return ENOMEM;
    while((bytesRead = fread(buffer, 1, bufSize, file)))
    {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    sha256_hash_string(hash, outputBuffer);
    fclose(file);
    free(buffer);
    return 0;
}

int hashingFile(boost::filesystem:: path p) {
    char outputBuffer[65];
    FILE *file = fopen(p.string().c_str(), "rb");
    if(!file) return -534;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char *buffer = static_cast<unsigned char *>(malloc(bufSize));
    int bytesRead = 0;
    if(!buffer) return ENOMEM;
    while((bytesRead = fread(buffer, 1, bufSize, file)))
    {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    sha256_hash_string(hash, outputBuffer);
    fclose(file);
    free(buffer);
    //std::cout<<"  Digest of "<<p<<" -> "<<outputBuffer<<"\n";
    return 0;
}

std::string get_hash_file(boost::filesystem:: path p) {
    char outputBuffer[65];
    FILE *file = fopen(p.string().c_str(), "rb");
    if(file) {

        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        const int bufSize = 32768;
        unsigned char *buffer = static_cast<unsigned char *>(malloc(bufSize));
        int bytesRead = 0;
        if(!buffer)  exit(1);
        while((bytesRead = fread(buffer, 1, bufSize, file)))
        {
            SHA256_Update(&sha256, buffer, bytesRead);
        }
        SHA256_Final(hash, &sha256);

        sha256_hash_string(hash, outputBuffer);
        fclose(file);
        free(buffer);
        //std::cout<<"  Digest of "<<p<<" -> "<<outputBuffer<<"\n";
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << (int)outputBuffer[i];
        }
        return std::move(ss.str());
    } }



class Sha256 {
private:
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    Sha256() { SHA256_Init(&sha256); }
    //static Sha256 *instance;

public:
    Sha256(const Sha256 &) = delete;
    Sha256(Sha256 &&) = delete;
    Sha256 &operator=(const Sha256 &) = delete;
    Sha256 &operator=(Sha256 &&) = delete;

    static std::string getSha256(const std::vector<char> &buf) {
        /*
        if (instance == nullptr) {
          instance = new Sha256();
        }
        */
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, buf.data(), buf.size());
        SHA256_Final(hash, &sha256);
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << (int)hash[i];
        }
        return std::move(ss.str());
    }


    static std::string getSha256(const std::shared_ptr<char[]> &buf,
                                 size_t size) {
        /*
        if (instance == nullptr) {
          instance = new Sha256();
        }
        */
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, buf.get(), size);
        SHA256_Final(hash, &sha256);
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return std::move(ss.str());
    }
};

