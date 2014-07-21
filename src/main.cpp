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

    yarrr::PhysicalParameters m_ship;
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
      m_players.erase( id );
    }

    void handle_incoming_messages()
    {
      the::net::Data message;
      while ( m_connection.receive( message ) )
      {
        yarrr::Event::Pointer event(
            yarrr::EventFactory::create( message ) );
        if ( !event )
        {
          continue;
        }

        m_dispatcher.polymorphic_dispatch( *event );
      }
    }

};

}

int main( int argc, char ** argv )
{
  yarrr::CallbackQueue callback_queue;
  std::unordered_map< int, ConnectionHandler::Pointer > connection_handlers;

  PlayerContainer players;

  the::time::Clock clock;
  the::net::Service network_service(
      [ &callback_queue, &players, &connection_handlers, &clock ]( the::net::Connection& connection )
      {
        callback_queue.push_back(
          [ &connection_handlers, &connection, &players ]()
          {
            ConnectionHandler::Pointer new_connection_handler( new ConnectionHandler( connection, players ) );
            connection_handlers.emplace( std::make_pair(
              connection.id,
              std::move( new_connection_handler ) ) );
          } );
        connection.register_task( the::net::NetworkTask::Pointer(
            new yarrr::clock_sync::Server< the::time::Clock, the::net::Connection >(
              clock,
              connection ) ) );
      },
      [ &callback_queue, &players, &connection_handlers ]( the::net::Connection& connection )
      {
        callback_queue.push_back(
          [ &players, &connection_handlers, &connection ]()
          {
            connection_handlers.erase( connection.id );
            for ( auto& player : players )
            {
              std::cout << "sending delete object" << std::endl;
              player.second->send( yarrr::DeleteObject( connection.id ).serialize() );
            }
          });
      } );

  network_service.listen_on( 2001 );
  network_service.start();

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );



  while ( true )
  {
    auto now( clock.now() );

    for ( auto& handler : connection_handlers )
    {
      handler.second->handle_incoming_messages();
    }

    std::vector< the::net::Data > ship_states;
    for ( auto& player : players )
    {
      player.second->update( now );
      ship_states.emplace_back( player.second->serialize() );
    }

    for ( auto& player : players )
    {
      for ( auto& ship_state : ship_states )
      {
        assert( player.second->send( the::net::Data( ship_state ) ) );
      }
    }

    callback_queue.process_callbacks();
    frequency_stabilizer.stabilize();
  }

  return 0;
}

