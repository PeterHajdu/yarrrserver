#pragma once

#include <thenet/test_socket.hpp>
#include <thenet/connection.hpp>

namespace test
{

class Connection
{
  public:
    Connection()
      : socket( test::Socket::create() )
      , connection( *socket )
      , wrapper( connection )
    {
    }

    bool has_no_data() const
    {
      connection.wake_up_on_network_thread();
      return socket->has_no_messages();
    }

    void flush_connection() const
    {
      connection.wake_up_on_network_thread();
      socket->sent_message();
    }

    std::unique_ptr< test::Socket > socket;
    mutable the::net::Connection connection;
    yarrrs::ConnectionWrapper wrapper;
};

}

