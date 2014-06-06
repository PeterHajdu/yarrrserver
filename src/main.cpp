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

  const int max_message_size( 1000 );
  std::array<char,max_message_size> messageBuffer;
  void data_available_on( yarrr::Socket& socket )
  {
    //todo: move reading and data buffering to lower layer
    const size_t length( ::read( socket.fd, &messageBuffer[0], max_message_size ) );
    std::cout << "data arrived: " << std::string( &messageBuffer[0], &messageBuffer[length] ) << std::endl;
  }
}

int main( int argc, char ** argv )
{
  yarrr::Ship ship;
  yarrr::time_step( ship );

  yarrr::SocketPool pool(
      new_connection,
      data_available_on
      );
  pool.listen( 2000 );
  pool.start();
  return 0;
}

