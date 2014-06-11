#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#include <yarrr/ship.hpp>
#include <thenet/socket_pool.hpp>


namespace
{
  void lost_connection( the::net::Socket& )
  {
    std::cout << "connection lost" << std::endl;
  }

  void data_available_on( the::net::Socket& socket, const char* message, size_t length )
  {
    std::cout << "data arrived: " << std::string( message, length ) << std::endl;
  }
}

class Player
{
  public:
    Player( the::net::Socket& socket )
      : m_socket( socket )
      , m_ship( "Ship: 1000 1100 1 2 3000 3" )
    {
    }

    void update()
    {
      yarrr::time_step( m_ship );
      const std::string ship_data( yarrr::serialize( m_ship ) );
      m_socket.send( ship_data.data(), ship_data.size() );
    }

  private:
    the::net::Socket& m_socket;
    yarrr::Ship m_ship;
};

int main( int argc, char ** argv )
{

  std::vector< Player > players;
  std::mutex players_mutex;

  the::net::SocketPool pool(
      [ &players, &players_mutex ]( the::net::Socket& socket )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.emplace_back( socket );
      },
      lost_connection,
      data_available_on );
  pool.listen( 2000 );

  std::thread network_thread( std::bind( &the::net::SocketPool::start, &pool ) );

  while ( true )
  {
    {
      std::lock_guard<std::mutex> lock( players_mutex );
      for ( auto& player : players )
      {
        player.update();
      }
    }

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  }

  pool.stop();
  network_thread.join();
  return 0;
}

