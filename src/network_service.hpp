#pragma once

#include <thectci/dispatcher.hpp>
#include <thenet/service.hpp>
#include <yarrr/connection_wrapper.hpp>
#include <yarrr/login.hpp>
#include <yarrr/callback_queue.hpp>
#include <yarrr/clock_synchronizer.hpp>

typedef yarrr::ConnectionWrapper<the::net::Connection> ConnectionWrapper;

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
    the::ctci::Dispatcher m_dispatcher;
    ConnectionWrapper& m_connection_wrapper;
    the::net::Connection& m_connection;
    const int m_id;

  public:
    typedef std::unique_ptr< LoginHandler > Pointer;
    LoginHandler( ConnectionWrapper& connection_wrapper );
    void handle_login_request( const yarrr::LoginRequest& request );
    ~LoginHandler();
};


class ConnectionBundle
{
  public:
    typedef std::unique_ptr< ConnectionBundle > Pointer;
    ConnectionBundle( the::net::Connection& connection )
      : connection_wrapper( connection )
      , login_handler( connection_wrapper )
    {
    }

    ConnectionWrapper connection_wrapper;
    LoginHandler login_handler;
};

namespace the
{
namespace time
{
class Clock;
}
}

class NetworkService
{
  public:
    NetworkService( the::time::Clock& clock );
    void process_network_events();

  private:
    void handle_new_connection( the::net::Connection& connection );
    void handle_new_connection_on_main_thread( the::net::Connection* connection );
    void handle_connection_lost( the::net::Connection& connection );
    void handle_connection_lost_on_main_thread( int connection_id );

    the::time::Clock& m_clock;
    the::net::Service m_network_service;
    yarrr::CallbackQueue m_callback_queue;
    std::unordered_map< int, ConnectionBundle::Pointer > m_connection_bundles;
};

