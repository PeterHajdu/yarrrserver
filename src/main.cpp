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

typedef std::unordered_map< int, Player::Pointer > PlayerContainer;

class LoginHandler
{
    the::ctci::Dispatcher m_dispatcher;
    PlayerContainer& m_players;
    ConnectionWrapper& m_connection_wrapper;
    the::net::Connection& m_connection;
    const int m_id;

  public:
    typedef std::unique_ptr< LoginHandler > Pointer;
    LoginHandler(
        ConnectionWrapper& connection_wrapper,
        PlayerContainer& players )
      : m_dispatcher()
      , m_players( players )
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
      m_players.emplace( std::make_pair(
            m_id,
            Player::Pointer( new Player(
                m_id,
                request.login_id(),
                m_connection_wrapper ) ) ) );
    }

    ~LoginHandler()
    {
      m_players.erase( m_id );
    }

};


class ConnectionBundle
{
  public:
    typedef std::unique_ptr< ConnectionBundle > Pointer;
    ConnectionBundle(
        the::net::Connection& connection,
        PlayerContainer& players )
      : connection_wrapper( connection )
      , login_handler( connection_wrapper, players )
    {
    }

    ConnectionWrapper connection_wrapper;
    LoginHandler login_handler;
};

}

int main( int argc, char ** argv )
{
  PlayerContainer players;

  yarrr::CallbackQueue callback_queue;
  std::unordered_map< int, ConnectionBundle::Pointer > connection_bundles;

  the::time::Clock clock;
  the::net::Service network_service(
      [ &callback_queue, &players, &connection_bundles, &clock ]( the::net::Connection& connection )
      {
        callback_queue.push_back(
          [ &connection_bundles, &connection, &players ]()
          {
            ConnectionBundle::Pointer new_connection_bundle( new ConnectionBundle( connection, players ) );
            connection_bundles.emplace( std::make_pair(
              connection.id,
              std::move( new_connection_bundle ) ) );
          } );
        connection.register_task( the::net::NetworkTask::Pointer(
            new yarrr::clock_sync::Server< the::time::Clock, the::net::Connection >(
              clock,
              connection ) ) );
      },
      [ &callback_queue, &players, &connection_bundles ]( the::net::Connection& connection )
      {
        callback_queue.push_back(
          [ &players, &connection_bundles, &connection ]()
          {
            connection_bundles.erase( connection.id );
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

    for ( auto& bundle : connection_bundles )
    {
      bundle.second->connection_wrapper.process_incoming_messages();
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

