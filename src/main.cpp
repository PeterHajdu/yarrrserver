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

  /*
  std::string dump( const the::net::Data& data )
  {
    std::string ret;
    for ( auto character : data )
    {
      ret += std::to_string( character ) + ",";
    }
    return ret;
  }
  */

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

    Player( int network_id, const std::string& name, the::ctci::Dispatcher& dispatcher )
      : id( network_id )
      , m_ship( random_ship() )
      , m_name( name )
    {
      m_ship.id = id;
      dispatcher.register_listener<yarrr::Command>( std::bind(
            &Player::handle_command, this, std::placeholders::_1 ) );
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

    void handle_command( const yarrr::Command& event )
    {
      yarrr::travel_in_time_to( event.timestamp(), m_ship );
      switch( event.id() )
      {
        case 1: thruster(); break;
        case 2: ccw(); break;
        case 3: cw(); break;
      }
    }

  private:
    void thruster()
    {
      const yarrr::Coordinate heading{
        static_cast< int64_t >( 40.0 * cos( m_ship.angle * 3.14 / 180.0 / 4.0 ) ),
        static_cast< int64_t >( 40.0 * sin( m_ship.angle * 3.14 / 180.0 / 4.0 ) ) };
      m_ship.velocity += heading;
    }

    void ccw()
    {
      m_ship.vangle -= 100;
    }

    void cw()
    {
      m_ship.vangle += 100;
    }

    yarrr::Object m_ship;
    std::string m_name;
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


    void handle( const yarrr::Event& event )
    {
      m_dispatcher.polymorphic_dispatch( event );
    }

    void handle_login_request( const yarrr::LoginRequest& request )
    {
      std::cout << "login_request arrived" << std::endl;
      m_connection.send( yarrr::LoginResponse( id ).serialize() );
      m_players.emplace( std::make_pair(
            id,
            Player::Pointer( new Player(
                id,
                request.login_id(),
                m_dispatcher ) ) ) );
    }

    ~ConnectionHandler()
    {
      std::cout << "connection lost" << std::endl;
    }
};


int main( int argc, char ** argv )
{
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

  the::ctci::Factory< yarrr::Event > event_factory;
  the::ctci::ExactCreator< yarrr::Event, yarrr::LoginRequest > login_request_creator;
  event_factory.register_creator( yarrr::LoginRequest::ctci, login_request_creator );

  the::ctci::ExactCreator< yarrr::Event, yarrr::Command > command_creator;
  event_factory.register_creator( yarrr::Command::ctci, command_creator );


  while ( true )
  {
    auto now( clock.now() );

    network_service.enumerate(
        [ &event_factory, &connection_handlers ]( the::net::Connection& connection )
        {
          the::net::Data message;
          while ( connection.receive( message ) )
          {
            yarrr::Event::Pointer event( event_factory.create( yarrr::extract<the::ctci::Id>(&message[0])) );
            if ( !event )
            {
              continue;
            }

            event->deserialize( message );
            connection_handlers[ connection.id ]->handle( *event );
          }
        } );

    std::vector< the::net::Data > ship_states;
    {
      for ( auto& player : players )
      {
        player.second->update( now );
        ship_states.emplace_back( player.second->serialize() );
      }
    }

    //todo: for each player, not connection!
    network_service.enumerate(
        [ &players, &ship_states ]( the::net::Connection& connection )
        {
          for ( auto& ship_state : ship_states )
          {
            assert( connection.send( the::net::Data( ship_state ) ) );
          }
        } );

    frequency_stabilizer.stabilize();
  }

  return 0;
}

