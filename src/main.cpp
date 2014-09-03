#include "network_service.hpp"
#include "player.hpp"

#include <yarrr/object_container.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>
#include <thectci/service_registry.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <theconf/configuration.hpp>
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


void
print_help_and_exit()
{
  std::cout << "usage: yarrrserver --port <port>" << std::endl;
  exit( 0 );
}


void
parse_and_handle_configuration( const the::conf::ParameterVector& parameters )
{
  the::conf::parse( parameters );
  if ( !the::conf::has( "port" ) )
  {
    print_help_and_exit();
  }
}

}

int main( int argc, char ** argv )
{
  parse_and_handle_configuration( the::conf::ParameterVector( argv, argv + argc ) );
  the::time::Clock clock;
  yarrrs::NetworkService network_service( clock );
  yarrr::ObjectContainer object_container;
  yarrrs::Players players( object_container );

  the::time::FrequencyStabilizer< 30, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    network_service.process_network_events();
    object_container.dispatch( yarrr::TimerUpdate( clock.now() ) );
    object_container.check_collision();
    players.broadcast( collect_update_messages_from( object_container ) );
    frequency_stabilizer.stabilize();

    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
  }

  return 0;
}

