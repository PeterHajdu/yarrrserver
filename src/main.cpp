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

  yarrr::SocketPool<TestMessageFactory> pool( 2000 );
  pool.listen();
  pool.start();
  return 0;
}

