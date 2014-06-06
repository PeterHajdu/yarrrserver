#include <iostream>

#include <yarrr/ship.hpp>
#include <yarrr/network.hpp>

int main( int argc, char ** argv )
{
  yarrr::Ship ship;
  yarrr::time_step( ship );

  yarrr::SocketPool pool;
  pool.listen( 2000 );
  pool.start();
  return 0;
}

