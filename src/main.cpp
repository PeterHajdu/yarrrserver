#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <algorithm>
#include <random>
#include <cassert>

#include <yarrr/object.hpp>
#include <yarrr/ship_control.hpp>
#include <yarrr/clock_synchronizer.hpp>
#include <yarrr/object_state_update.hpp>
#include <yarrr/login.hpp>
#include <yarrr/command.hpp>

#include <thenet/service.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>

#include <thectci/factory.hpp>
#include <thectci/dispatcher.hpp>

namespace
{
  the::ctci::Factory< yarrr::Event > event_factory;
  the::ctci::ExactCreator< yarrr::Event, yarrr::LoginRequest > login_request_creator;
  the::ctci::ExactCreator< yarrr::Event, yarrr::Command > command_creator;

  std::random_device rd;
  std::mt19937 gen( rd() );
  std::uniform_int_distribution<> x_dis( 300, 500 );
  std::uniform_int_distribution<> y_dis( 300, 400 );
  std::uniform_int_distribution<> angle_dis( 0, 360 * 4 );
  std::uniform_int_distribution<> velocity_dis( -100, +100 );
  std::uniform_int_distribution<> ang_velocity_dis( -10, +10 );

  yarrr::Object random_ship()
  {
    yarrr::Object ship;
    ship.coordinate.x = x_dis( gen );
    ship.coordinate.y = y_dis( gen );
    ship.angle = angle_dis( gen );

    ship.velocity.x = velocity_dis( gen );
    ship.velocity.y = velocity_dis( gen );

    ship.vangle = ang_velocity_dis( gen );

    return ship;
  }
}

class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player(
        int network_id,
        const std::string& name,
        the::ctci::Dispatcher& dispatcher,
        the::net::Connection& connection )

      : id( network_id )
      , m_ship( random_ship() )
      , m_ship_control( m_ship )
      , m_name( name )
      , m_connection( connection )
    {
      m_ship.id = id;
      dispatcher.register_listener<yarrr::Command>( std::bind(
            &yarrr::ShipControl::handle_command, m_ship_control, std::placeholders::_1 ) );
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

    yarrr::Object m_ship;
    yarrr::ShipControl m_ship_control;
    std::string m_name;
    the::net::Connection& m_connection;
};

typedef std::unordered_map< int, Player::Pointer > PlayerContainer;

class ConnectionHandler
{
    the::ctci::Dispatcher m_dispatcher;
    PlayerContainer& m_players;
    the::net::Connection& m_connection;

  public:
    typedef std::unique_ptr< ConnectionHandler > Pointer;
    ConnectionHandler(
        the::net::Connection& connection,
        PlayerContainer& players )
      : m_dispatcher()
      , m_players( players )
      , m_connection( connection )
      , id( connection.id )
    {
      m_dispatcher.register_listener<yarrr::LoginRequest>(
          std::bind( &ConnectionHandler::handle_login_request, this, std::placeholders::_1 ) );
    }

    const int id;

    void handle_login_request( const yarrr::LoginRequest& request )
    {
      std::cout << "login_request arrived" << std::endl;
      m_connection.send( yarrr::LoginResponse( id ).serialize() );
      m_players.emplace( std::make_pair(
            id,
            Player::Pointer( new Player(
                id,
                request.login_id(),
                m_dispatcher,
                m_connection ) ) ) );
    }

    ~ConnectionHandler()
    {
      std::cout << "connection lost" << std::endl;
    }

    void handle_incoming_messages()
    {
      the::net::Data message;
      while ( m_connection.receive( message ) )
      {
        yarrr::Event::Pointer event( event_factory.create( yarrr::extract<the::ctci::Id>(&message[0])) );
        if ( !event )
        {
          continue;
        }

        event->deserialize( message );
        m_dispatcher.polymorphic_dispatch( *event );
      }
    }

};


int main( int argc, char ** argv )
{
  event_factory.register_creator( yarrr::LoginRequest::ctci, login_request_creator );
  event_factory.register_creator( yarrr::Command::ctci, command_creator );
  std::unordered_map< int, ConnectionHandler::Pointer > connection_handlers;
  std::mutex connection_handlers_mutex;

  PlayerContainer players;

  the::time::Clock clock;
  the::net::Service network_service(
      [ &players, &connection_handlers, &connection_handlers_mutex, &clock ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( connection_handlers_mutex );
        ConnectionHandler::Pointer new_connection_handler( new ConnectionHandler( connection, players ) );
        connection_handlers.emplace( std::make_pair(
          connection.id,
          std::move( new_connection_handler ) ) );

        connection.register_task( the::net::NetworkTask::Pointer(
            new yarrr::clock_sync::Server< the::time::Clock, the::net::Connection >(
              clock,
              connection ) ) );
      },
      [ &connection_handlers, &connection_handlers_mutex ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( connection_handlers_mutex );
        connection_handlers.erase( connection.id );
      } );

  network_service.listen_on( 2001 );
  network_service.start();

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );



  while ( true )
  {
    auto now( clock.now() );

    {
      std::lock_guard< std::mutex > guard_connection_handlers( connection_handlers_mutex );
      for ( auto& handler : connection_handlers )
      {
        handler.second->handle_incoming_messages();
      }
    }

    std::vector< the::net::Data > ship_states;
    {
      for ( auto& player : players )
      {
        player.second->update( now );
        ship_states.emplace_back( player.second->serialize() );
      }
    }

    for ( auto& player : players )
    {
      for ( auto& ship_state : ship_states )
      {
        assert( player.second->send( the::net::Data( ship_state ) ) );
      }
    }

    frequency_stabilizer.stabilize();
  }

  return 0;
}

