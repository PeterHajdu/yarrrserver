#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <algorithm>
#include <random>

#include <yarrr/ship.hpp>
#include <thenet/service.hpp>

namespace
{
  std::random_device rd;
  std::mt19937 gen( rd() );
  std::uniform_int_distribution<> coordinate_dis( 200, 800 );
  std::uniform_int_distribution<> angle_dis( 0, 360 );
  std::uniform_int_distribution<> velocity_dis( -3, +3 );

  yarrr::Ship random_ship()
  {
    yarrr::Ship ship;
    ship.coordinate.x = coordinate_dis( gen );
    ship.coordinate.y = coordinate_dis( gen );
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

    void update()
    {
      yarrr::time_step( m_ship );
    }

    the::net::Data serialize() const
    {
      const std::string ship_data( yarrr::serialize( m_ship ) );
      return the::net::Data( begin( ship_data ) , end( ship_data ) );
    }

  private:
    yarrr::Ship m_ship;
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

  while ( true )
  {
    std::vector< the::net::Data > ship_states;
    {
      std::lock_guard<std::mutex> lock( players_mutex );
      for ( auto& player : players )
      {
        player.second->update();
        ship_states.emplace_back( player.second->serialize() );
      }
    }

    network_service.enumerate(
        [ &ship_states ]( the::net::Connection& connection )
        {
          for ( auto& ship_state : ship_states )
          {
            connection.send( the::net::Data( ship_state ) );
          }

          the::net::Data message;
          while ( connection.receive( message ) )
          {
            std::cout << "message arrived from player: " << connection.id << " " <<
            std::string( begin( message ), end( message ) ) << std::endl;
          }
        } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
  }

  return 0;
}

