#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <algorithm>
#include <random>
#include <cassert>

#include <yarrr/object.hpp>
#include <yarrr/ship_control.hpp>
#include <yarrr/clock_synchronizer.hpp>
#include <yarrr/object_state_update.hpp>
#include <yarrr/login.hpp>
#include <yarrr/command.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/event_factory.hpp>
#include <yarrr/callback_queue.hpp>
#include <yarrr/connection_wrapper.hpp>

#include <thenet/service.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>

#include <thectci/dispatcher.hpp>

namespace
{
  std::random_device rd;
  std::mt19937 gen( rd() );
  std::uniform_int_distribution<> x_dis( 300, 500 );
  std::uniform_int_distribution<> y_dis( 300, 400 );
  std::uniform_int_distribution<> angle_dis( 0, 360 * 4 );
  std::uniform_int_distribution<> velocity_dis( -100, +100 );
  std::uniform_int_distribution<> ang_velocity_dis( -10, +10 );

  yarrr::PhysicalParameters random_ship()
  {
    yarrr::PhysicalParameters ship;
    ship.coordinate.x = x_dis( gen );
    ship.coordinate.y = y_dis( gen );
    ship.angle = angle_dis( gen );

    ship.velocity.x = velocity_dis( gen );
    ship.velocity.y = velocity_dis( gen );

    ship.vangle = ang_velocity_dis( gen );

    return ship;
  }


  typedef yarrr::ConnectionWrapper<the::net::Connection> ConnectionWrapper;

the::ctci::Dispatcher local_event_dispatcher;

class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player(
        int network_id,
        const std::string& name,
        ConnectionWrapper& connection_wrapper )

      : id( network_id )
      , m_ship( random_ship() )
      , m_ship_control( m_ship )
      , m_name( name )
      , m_connection( connection_wrapper.connection )
    {
      m_ship.id = id;
      m_dispatcher.register_listener<yarrr::Command>( std::bind(
            &yarrr::ShipControl::handle_command, m_ship_control, std::placeholders::_1 ) );
      connection_wrapper.register_dispatcher( m_dispatcher );
    }

    const int id;

    void update( const the::time::Clock::Time& timestamp )
    {
      yarrr::travel_in_time_to( timestamp, m_ship );
    }

    the::net::Data serialize() const
    {
      return yarrr::ObjectStateUpdate( m_ship ).serialize();
    }

    bool send( yarrr::Data&& message )
    {
      return m_connection.send( std::move( message ) );
    }

  private:

    the::ctci::Dispatcher m_dispatcher;
    yarrr::PhysicalParameters m_ship;
    yarrr::ShipControl m_ship_control;
    std::string m_name;
    the::net::Connection& m_connection;
};

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

class Players
{
  public:
    Players()
    {
      local_event_dispatcher.register_listener< PlayerLoggedIn >(
          std::bind( &Players::handle_player_login, this, std::placeholders::_1 ) );
      local_event_dispatcher.register_listener< PlayerLoggedOut >(
          std::bind( &Players::handle_player_logout, this, std::placeholders::_1 ) );
    }

    void update_to( the::time::Time& timestamp )
    {
      m_last_ship_states.clear();
      for ( auto& player : m_players )
      {
        player.second->update( timestamp );
        m_last_ship_states.emplace_back( player.second->serialize() );
      }

      broadcast_ship_states();
    }

  private:
    void broadcast_ship_states() const
    {
      for ( auto& player : m_players )
      {
        for ( auto& ship_state : m_last_ship_states )
        {
          assert( player.second->send( the::net::Data( ship_state ) ) );
        }
      }
    }

    void handle_player_login( const PlayerLoggedIn& login )
    {
      m_players.emplace( std::make_pair(
            login.id,
            Player::Pointer( new Player(
                login.id,
                login.name,
                login.connection_wrapper ) ) ) );
    }

    void handle_player_logout( const PlayerLoggedOut& logout )
    {
      m_players.erase( logout.id );
      for ( auto& player : m_players )
      {
        player.second->send( yarrr::DeleteObject( logout.id ).serialize() );
      }
    }

    std::vector< the::net::Data > m_last_ship_states;
    typedef std::unordered_map< int, Player::Pointer > PlayerContainer;
    PlayerContainer m_players;
};

class LoginHandler
{
    the::ctci::Dispatcher m_dispatcher;
    ConnectionWrapper& m_connection_wrapper;
    the::net::Connection& m_connection;
    const int m_id;

  public:
    typedef std::unique_ptr< LoginHandler > Pointer;
    LoginHandler( ConnectionWrapper& connection_wrapper )
      : m_dispatcher()
      , m_connection_wrapper( connection_wrapper )
      , m_connection( connection_wrapper.connection )
      , m_id( m_connection.id )
    {
      m_dispatcher.register_listener<yarrr::LoginRequest>(
          std::bind( &LoginHandler::handle_login_request, this, std::placeholders::_1 ) );
      connection_wrapper.register_dispatcher( m_dispatcher );
    }

    void handle_login_request( const yarrr::LoginRequest& request )
    {
      m_connection.send( yarrr::LoginResponse( m_id ).serialize() );
      local_event_dispatcher.dispatch( PlayerLoggedIn( m_connection_wrapper, m_id, request.login_id() ) );
    }

    ~LoginHandler()
    {
      local_event_dispatcher.dispatch( PlayerLoggedOut( m_id ) );
    }

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

class NetworkService
{
  public:
    NetworkService( the::time::Clock& clock )
      : m_clock( clock )
      , m_network_service(
          std::bind( &NetworkService::handle_new_connection, this, std::placeholders::_1 ),
          std::bind( &NetworkService::handle_connection_lost, this, std::placeholders::_1 ) )
    {
      m_network_service.listen_on( 2001 );
      m_network_service.start();
    }

    void handle_new_connection( the::net::Connection& connection )
    {
      connection.register_task( the::net::NetworkTask::Pointer(
            new yarrr::clock_sync::Server< the::time::Clock, the::net::Connection >(
              m_clock,
              connection ) ) );

      //todo: connection might be deleted before it gets added on the main thread
      //todo: should we lock?
      m_callback_queue.push_back( std::bind(
            &NetworkService::handle_new_connection_on_main_thread, this, &connection ) );
    }

    void handle_new_connection_on_main_thread( the::net::Connection* connection )
    {
      ConnectionBundle::Pointer new_connection_bundle( new ConnectionBundle( *connection ) );
      m_connection_bundles.emplace(
            connection->id,
            std::move( new_connection_bundle ) );
    }

    void handle_connection_lost( the::net::Connection& connection )
    {
      m_callback_queue.push_back( std::bind(
            &NetworkService::handle_connection_lost_on_main_thread, this, connection.id ) );
    }

    void handle_connection_lost_on_main_thread( int connection_id )
    {
      m_connection_bundles.erase( connection_id );
    }

    void process_network_events()
    {
      m_callback_queue.process_callbacks();
      for ( auto& bundle : m_connection_bundles )
      {
        bundle.second->connection_wrapper.process_incoming_messages();
      }
    }

  private:
    the::time::Clock& m_clock;
    the::net::Service m_network_service;
    yarrr::CallbackQueue m_callback_queue;
    std::unordered_map< int, ConnectionBundle::Pointer > m_connection_bundles;
};

}

int main( int argc, char ** argv )
{
  the::time::Clock clock;
  NetworkService network_service( clock );
  Players players;

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    auto now( clock.now() );

    network_service.process_network_events();

    players.update_to( now );
    frequency_stabilizer.stabilize();
  }

  return 0;
}

