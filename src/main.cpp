#include <iostream>

#include <yarrr/ship.hpp>
#include <yarrr/network.hpp>

namespace
{
  void new_connection( yarrr::Socket& )
  {
    std::cout << "new connection established" << std::endl;
  }

  void data_available_on( yarrr::Socket& socket )
  {
    std::cout << "data is available" << std::endl;
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

