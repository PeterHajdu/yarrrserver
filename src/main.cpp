#include "network_service.hpp"
#include "player.hpp"

#include <yarrr/object_container.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>
#include <thectci/service_registry.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <iostream>

namespace
{
  std::vector< yarrr::Data >
  collect_update_messages_from( const yarrr::ObjectContainer& objects )
  {
    std::vector< yarrr::ObjectUpdate::Pointer > object_updates( objects.generate_object_updates() );
    std::vector< yarrr::Data > update_messages;
    std::transform(
        std::begin( object_updates ), std::end( object_updates ),
        std::back_inserter( update_messages ),
        []( const yarrr::ObjectUpdate::Pointer& update )
        {
          return update->serialize();
        } );
    return update_messages;
  }
}

int main( int argc, char ** argv )
{
  the::time::Clock clock;
  NetworkService network_service( clock );
  yarrr::ObjectContainer object_container;
  Players players( object_container );

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    network_service.process_network_events();
    object_container.dispatch( yarrr::TimerUpdate( clock.now() ) );
    players.broadcast( collect_update_messages_from( object_container ) );
    frequency_stabilizer.stabilize();

    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
  }

  return 0;
}

