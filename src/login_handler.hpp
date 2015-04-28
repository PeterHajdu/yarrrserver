#pragma once

#include <yarrr/connection_wrapper.hpp>
#include <thenet/connection.hpp>

namespace yarrr
{

class Command;
class Db;

}

namespace the
{

namespace ctci
{

class Dispatcher;

}

}

namespace yarrrs
{

using ConnectionWrapper = yarrr::ConnectionWrapper<the::net::Connection>;

class PlayerLoggedIn
{
  public:
    add_ctci( "player_logged_in" );
    PlayerLoggedIn(
        ConnectionWrapper& connection_wrapper,
        int id,
        const std::string& name )
      : connection_wrapper( connection_wrapper )
      , id( id )
      , name( name )
    {
    }

    ConnectionWrapper& connection_wrapper;
    const int id;
    const std::string& name;
};

class PlayerLoggedOut
{
  public:
    add_ctci( "player_logged_out" );
    PlayerLoggedOut( int id )
      : id( id )
    {
    }

    const int id;
};

class LoginHandler
{
  public:
    LoginHandler( ConnectionWrapper& , the::ctci::Dispatcher& );
    ~LoginHandler();

  private:
    void handle_registration_request( const yarrr::Command& request );
    void handle_login_request( const yarrr::Command& request );
    void handle_authentication_response( const yarrr::Command& request ) const;
    void log_in() const;
    void send_login_error_message( const std::string& additional_information ) const;

    ConnectionWrapper& m_connection_wrapper;
    the::net::Connection::Pointer m_connection;
    const int m_id;
    the::ctci::SmartListener m_command_callback;
    the::ctci::Dispatcher& m_dispatcher;
    std::string m_username;
    std::string m_challenge;
    bool m_was_authentication_request_sent_out;
};

}

