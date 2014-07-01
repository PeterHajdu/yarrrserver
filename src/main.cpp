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
#include <thenet/service.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>

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

    Player( int network_id )
      : id( network_id )
      , m_ship( random_ship() )
    {
      m_ship.id = id;
    }
    const int id;

    void update( const the::time::Clock::Time& timestamp )
    {
      yarrr::travel_in_time_to( timestamp, m_ship );
    }

    the::net::Data serialize() const
    {
      const std::string ship_data( yarrr::serialize( m_ship ) );
      return the::net::Data( begin( ship_data ) , end( ship_data ) );
    }

    void command( char cmd, const the::time::Time timestamp )
    {
      yarrr::travel_in_time_to( timestamp, m_ship );
      switch( cmd )
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
};

int main( int argc, char ** argv )
{
  std::unordered_map< int, Player::Pointer > players;
  std::mutex players_mutex;

  the::time::Clock clock;
  the::net::Service network_service(
      [ &players, &players_mutex, &clock ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        Player::Pointer new_player( new Player( connection.id ) );
        players.emplace( std::make_pair(
          connection.id,
          std::move( new_player ) ) );

        connection.register_task( the::net::NetworkTask::Pointer(
            new yarrr::clock_sync::Server< the::time::Clock, the::net::Connection >(
              clock,
              connection ) ) );
      },
      [ &players, &players_mutex ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.erase( connection.id );
      } );

  network_service.listen_on( 2001 );
  network_service.start();

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );

  while ( true )
  {
    auto now( clock.now() );

    network_service.enumerate(
        [ &players_mutex, &players ]( the::net::Connection& connection )
        {
          std::lock_guard<std::mutex> lock( players_mutex );
          the::net::Data message;
          Player& current_player( *players[ connection.id ] );
          while ( connection.receive( message ) )
          {
            if ( message[ 0 ] == 2 )
            {
              current_player.command( message[1], *reinterpret_cast<const uint64_t*>(&message[2]) );
            }
          }
        } );

    std::vector< the::net::Data > ship_states;
    {
      std::lock_guard<std::mutex> lock( players_mutex );
      for ( auto& player : players )
      {
        player.second->update( now );
        ship_states.emplace_back( player.second->serialize() );
      }
    }

    network_service.enumerate(
        [ &players_mutex, &players, &ship_states ]( the::net::Connection& connection )
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

