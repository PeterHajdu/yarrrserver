#include "network_service.hpp"
#include "player.hpp"

#include <yarrr/object_container.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>
#include <thectci/service_registry.hpp>

#include <thelog/logger.hpp>
#include <iostream>

int main( int argc, char ** argv )
{
  the::log::Logger::add_channel( std::cout );
  the::time::Clock clock;
  NetworkService network_service( clock );
  yarrr::ObjectContainer object_container;
  Players players( object_container );

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    network_service.process_network_events();
    object_container.dispatch( yarrr::TimerUpdate( clock.now() ) );
    //todo:collect object updates and broadcast it
    frequency_stabilizer.stabilize();
  }

  return 0;
}

