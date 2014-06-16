#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <algorithm>

#include <yarrr/ship.hpp>
#include <thenet/socket_pool.hpp>
#include <thenet/connection_pool.hpp>

class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player( int network_id )
      : id( network_id )
      , m_ship( "Ship: 1000 1100 1 2 3000 3" )
    {
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

  the::net::ConnectionPool cpool(
      [ &players, &players_mutex ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        Player::Pointer new_player( new Player( connection.id ) );
        players.emplace( std::make_pair(
          connection.id,
          std::move( new_player ) ) );
      },
      [ &players, &players_mutex ]( the::net::Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.erase( connection.id );
      } );

  the::net::SocketPool pool(
      std::bind( &the::net::ConnectionPool::on_new_socket, &cpool, std::placeholders::_1 ),
      std::bind( &the::net::ConnectionPool::on_socket_lost, &cpool, std::placeholders::_1 ),
      std::bind( &the::net::ConnectionPool::on_data_available, &cpool,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) );

  pool.listen( 2000 );

  std::thread network_thread(
      [ &cpool, &pool, &players_mutex, &players ]()
      {
        while ( true )
        {
          const int ten_milliseconds( 10 );
          pool.run_for( ten_milliseconds );
          {
            std::lock_guard<std::mutex> lock( players_mutex );
            cpool.wake_up_on_network_thread();
          }
        }
      } );

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

    cpool.enumerate(
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

  network_thread.join();
  return 0;
}

