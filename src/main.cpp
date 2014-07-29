#include "network_service.hpp"

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
#include <yarrr/object_state_update.hpp>
#include <yarrr/command.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/event_factory.hpp>

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

