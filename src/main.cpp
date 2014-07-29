#include "network_service.hpp"
#include "player.hpp"

#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>

int main( int argc, char ** argv )
{
  the::time::Clock clock;
  NetworkService network_service( clock );
  Players players;

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    network_service.process_network_events();
    frequency_stabilizer.stabilize();
  }

  return 0;
}

