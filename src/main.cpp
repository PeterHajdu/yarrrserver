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

int main( int argc, char ** argv )
{

  std::vector< std::reference_wrapper< yarrr::Socket > > clients;
  std::mutex clients_mutex;

  yarrr::SocketPool pool(
      [ &clients, &clients_mutex ]( yarrr::Socket& socket )
      {
        std::lock_guard<std::mutex> lock( clients_mutex );
        clients.emplace_back( socket );
      },
      lost_connection,
      data_available_on );
  pool.listen( 2000 );

  std::thread network_thread( std::bind( &yarrr::SocketPool::start, &pool ) );

  while ( true )
  {
    {
      std::lock_guard<std::mutex> lock( clients_mutex );
      for ( auto& client : clients )
      {
        const std::string message( "hello dude" );
        client.get().send( message.data(), message.length() );
      }
    }

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  }

  pool.stop();
  network_thread.join();
  return 0;
}

