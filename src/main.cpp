#include "network_service.hpp"
#include "player.hpp"
#include "object_container.hpp"

#include <yarrr/basic_behaviors.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>
#include <thectci/service_registry.hpp>

int main( int argc, char ** argv )
{
  the::time::Clock clock;
  ObjectContainer& object_container( the::ctci::service< ObjectContainer >() );
  NetworkService network_service( clock );
  Players players;

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    network_service.process_network_events();

    object_container.dispatch( yarrr::TimerUpdate( clock.now() ) );

    yarrr::SerializePhysicalParameter::SerializedDataBuffer ship_states;
    object_container.dispatch( yarrr::SerializePhysicalParameter( ship_states ) );

    players.broadcast( ship_states );

    frequency_stabilizer.stabilize();
  }

  return 0;
}

