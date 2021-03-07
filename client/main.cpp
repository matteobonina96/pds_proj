#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include "ClientC.h"




int main(int argc, char* argv[])
{
    if(argc!=2)  {
        cout<<"Devi specificare il path da monitorare..."<<std::endl;
        return 1;
    }
    std::string user_path = argv[1];


    // Getting username from user
    ClientC connection{user_path,9994};




    return 0;
}