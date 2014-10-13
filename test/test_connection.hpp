#pragma once

#include <thenet/test_socket.hpp>
#include <thenet/connection.hpp>
#include <thenet/packetizer.hpp>

namespace test
{

class Connection
{
  public:
    Connection()
      : socket( test::Socket::create() )
      , connection( *socket )
      , wrapper( connection )
      , m_packetizer( *this )
    {
    }

    void process_messages()
    {
      connection.wake_up_on_network_thread();
      const yarrr::Data buffer( socket->sent_message() );
      m_packetizer.receive( &buffer[0], buffer.size() );
    }

    bool has_no_data()
    {
      process_messages();
      return sent_messages.empty();
    }

    std::vector< yarrr::Data > sent_messages;
    void message_from_network( yarrr::Data&& message )
    {
      sent_messages.push_back( message );
    }

    void drop()
    {
    }

    void flush_connection() const
    {
      connection.wake_up_on_network_thread();
      socket->sent_message();
    }

    std::unique_ptr< test::Socket > socket;
    mutable the::net::Connection connection;
    yarrrs::ConnectionWrapper wrapper;

  private:
    the::net::packetizer::Incoming< Connection > m_packetizer;
};

}

