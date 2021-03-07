#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include "Server.cpp"


int main(int argc, char* argv[])
{
    try
    {

        boost::asio::io_service io_service;
        server s(io_service, 9994);
        io_service.run();

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
