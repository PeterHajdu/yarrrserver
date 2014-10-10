#pragma once

#include <thenet/socket.hpp>
#include <thenet/connection.hpp>

namespace test
{

class Socket : public the::net::Socket
{
  public:
    Socket()
      : the::net::Socket( 0 )
    {
    }

    virtual void drop() override {}
    virtual void handle_event() override {}
    virtual ~Socket() = default;
};

class Connection
{
  public:
    Connection()
      : socket()
      , connection( socket )
      , wrapper( connection )
    {
    }

    Socket socket;
    the::net::Connection connection;
    yarrrs::ConnectionWrapper wrapper;
};

}

