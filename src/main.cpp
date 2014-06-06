#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#include <yarrr/ship.hpp>
#include <yarrr/socket_pool.hpp>


namespace
{
  void lost_connection( yarrr::Socket& )
  {
    std::cout << "connection lost" << std::endl;
  }

  void data_available_on( yarrr::Socket& socket, const char* message, size_t length )
  {
    std::cout << "data arrived: " << std::string( message, length ) << std::endl;
  }
}

class Player
{
  public:
    Player( yarrr::Socket& socket )
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
    yarrr::Socket& m_socket;
    yarrr::Ship m_ship;
};

int main( int argc, char ** argv )
{

  std::vector< Player > players;
  std::mutex players_mutex;

  yarrr::SocketPool pool(
      [ &players, &players_mutex ]( yarrr::Socket& socket )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.emplace_back( socket );
      },
      lost_connection,
      data_available_on );
  pool.listen( 2000 );

  std::thread network_thread( std::bind( &yarrr::SocketPool::start, &pool ) );

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

