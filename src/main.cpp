#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#include <yarrr/ship.hpp>
#include <thenet/socket_pool.hpp>
#include <thenet/message_queue.hpp>


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
    typedef std::unique_ptr< Player > Pointer;

    Player( the::net::Socket& socket )
      : m_socket( socket )
      , m_message_queue(
          std::bind(
            &the::net::Socket::send,
            &m_socket,
            std::placeholders::_1, std::placeholders::_2 ) )
      , m_ship( "Ship: 1000 1100 1 2 3000 3" )
    {
    }

    void update()
    {
      yarrr::time_step( m_ship );
      const std::string ship_data( yarrr::serialize( m_ship ) );
      m_message_queue.send( { begin( ship_data ), end( ship_data ) } );
    }

  private:
    the::net::Socket& m_socket;
    the::net::MessageQueue m_message_queue;
    yarrr::Ship m_ship;
};

int main( int argc, char ** argv )
{

  std::vector< Player::Pointer > players;
  std::mutex players_mutex;

  the::net::SocketPool pool(
      [ &players, &players_mutex ]( the::net::Socket& socket )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.emplace_back( new Player( socket ) );
      },
      lost_connection,
      data_available_on );
  pool.listen( 2000 );

  std::thread network_thread(
      [ &pool ]()
      {
        while ( true )
        {
          const int ten_milliseconds( 10 );
          pool.run_for( ten_milliseconds );
        }
      } );

  while ( true )
  {
    {
      std::lock_guard<std::mutex> lock( players_mutex );
      for ( auto& player : players )
      {
        player->update();
      }
    }

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  }

  network_thread.join();
  return 0;
}

