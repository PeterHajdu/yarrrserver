#include <iostream>

#include <yarrr/ship.hpp>
#include <yarrr/network.hpp>

class TestMessageFactory
{
  public:
    static void received_data()
    {
      std::cout << "data is received" << std::endl;
    }
};

int main( int argc, char ** argv )
{
  yarrr::Ship ship;
  yarrr::time_step( ship );

  yarrr::SocketPool<TestMessageFactory> pool;
  pool.listen( 2000 );
  pool.start();
  return 0;
}

