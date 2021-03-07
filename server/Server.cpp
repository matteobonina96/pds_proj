#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "ServerC.h"

using boost::asio::ip::tcp;

class server
{
public:
    server(boost::asio::io_service& io_service, short port)
            : io_service_(io_service),
              acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        cout<<"Benvenuto in REMOTE-BACKUP - Progetto realizzato da Matteo,Raffaella e Desire\n";
        cout<<"In attesa di connessione\n";
        ServerC* new_session = new ServerC(io_service_);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&server::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(ServerC* new_session,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            std::thread t([this,new_session]() {
                new_session->start();
            });
            t.detach();
            //new_session->start();
            new_session = new ServerC(io_service_);
            acceptor_.async_accept(new_session->socket(),
                                   boost::bind(&server::handle_accept, this, new_session,
                                               boost::asio::placeholders::error));
        }
        else
        {
            delete new_session;
        }
    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
};