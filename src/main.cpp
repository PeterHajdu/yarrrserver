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
#include <thenet/service.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>

namespace
{
  std::random_device rd;
  std::mt19937 gen( rd() );
  std::uniform_int_distribution<> x_dis( 300, 500 );
  std::uniform_int_distribution<> y_dis( 300, 400 );
  std::uniform_int_distribution<> angle_dis( 0, 360 );
  std::uniform_int_distribution<> velocity_dis( -100, +100 );

  yarrr::Object random_ship()
  {
    yarrr::Object ship;
    ship.coordinate.x = x_dis( gen );
    ship.coordinate.y = y_dis( gen );
    ship.angle = angle_dis( gen );

    ship.velocity.x = velocity_dis( gen );
    ship.velocity.y = velocity_dis( gen );

    ship.vangle = velocity_dis( gen );

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
      yarrr::advance_time_to( timestamp, m_ship );
    }

    the::net::Data serialize() const
    {
      const std::string ship_data( yarrr::serialize( m_ship ) );
      return the::net::Data( begin( ship_data ) , end( ship_data ) );
    }

  private:
    yarrr::Object m_ship;
};

int main( int argc, char ** argv )
{
  std::unordered_map< int, Player::Pointer > players;
  std::mutex players_mutex;

  the::net::Service network_service(
      [ &players, &players_mutex ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        Player::Pointer new_player( new Player( connection.id ) );
        players.emplace( std::make_pair(
          connection.id,
          std::move( new_player ) ) );
        std::cout << "new connection " << connection.id << std::endl;
      },
      [ &players, &players_mutex ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.erase( connection.id );
        std::cout << "connection lost " << connection.id << std::endl;
      } );

  network_service.listen_on( 2000 );
  network_service.start();

  the::time::Clock clock;
  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    auto now( clock.now() );
    std::vector< the::net::Data > ship_states;
    {
      std::lock_guard<std::mutex> lock( players_mutex );
      for ( auto& player : players )
      {
        player.second->update( now );
        ship_states.emplace_back( player.second->serialize() );
        std::cout << "add ship state " << player.second->id << std::endl;
      }
    }

    network_service.enumerate(
        [ &ship_states ]( the::net::Connection& connection )
        {
          for ( auto& ship_state : ship_states )
          {
            std::cout << "send ship state " << std::string( begin( ship_state ), end( ship_state ) ) << " to " << connection.id << std::endl;
            assert( connection.send( the::net::Data( ship_state ) ) );
          }

          the::net::Data message;
          while ( connection.receive( message ) )
          {
            std::cout << "message arrived from player: " << connection.id << " " <<
            std::string( begin( message ), end( message ) ) << std::endl;
          }
        } );

    frequency_stabilizer.stabilize();
  }

  return 0;
}

