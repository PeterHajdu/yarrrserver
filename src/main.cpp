#include <iostream>
#include <array>

#include <yarrr/ship.hpp>
#include <yarrr/network.hpp>

//todo: remove when read is removed
#include <unistd.h>

namespace
{
  void new_connection( yarrr::Socket& )
  {
    std::cout << "new connection established" << std::endl;
  }

  void lost_connection( yarrr::Socket& )
  {
    std::cout << "connection lost" << std::endl;
  }

  void data_available_on( yarrr::Socket& socket, char* message, size_t length )
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
  pool.start();
  return 0;
}

