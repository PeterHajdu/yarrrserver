#include <iostream>
#include <array>
#include <thread>
#include <chrono>

#include <yarrr/ship.hpp>
#include <yarrr/network.hpp>


namespace
{
  void new_connection( yarrr::Socket& socket )
  {
    std::cout << "new connection established" << std::endl;
    const std::string helloMessage( "hello world" );
    socket.send( helloMessage.data(), helloMessage.size() );
  }

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
  yarrr::SocketPool pool(
      new_connection,
      lost_connection,
      data_available_on );
  pool.listen( 2000 );

  std::thread network_thread( std::bind( &yarrr::SocketPool::start, &pool ) );

  std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

  pool.stop();
  network_thread.join();
  return 0;
}

