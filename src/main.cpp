#include "network_service.hpp"
#include "duck_hunt.hpp"
#include "player.hpp"
#include "world.hpp"
#include "notifier.hpp"
#include "object_factory.hpp"
#include "lua_setup.hpp"

#include <yarrr/object_container.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>
#include <yarrr/resources.hpp>
#include <thectci/service_registry.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <theconf/configuration.hpp>
#include <iostream>
#include <fstream>

#include <stdlib.h>

namespace
{

void
send_update_messages_from(
    const yarrr::ObjectContainer& objects,
    yarrrs::Player::Container& players )
{
  std::vector< yarrr::ObjectUpdate::Pointer > object_updates( objects.generate_object_updates() );
  std::vector< yarrr::Data > update_messages;
  for ( const auto& update : object_updates )
  {
    const yarrr::Data message( update->serialize() );

    for ( const auto& player : players )
    {
      player.second->send( yarrr::Data( message ) );
    }

  }
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

std::ostream&
create_notification_stream()
{
  static std::ofstream notification_stream(
      the::conf::has( "notify" ) ?
        the::conf::get< std::string >( "notify" ).c_str() :
        "dummy_notification_file" );
  return notification_stream;
}

}

int main( int argc, char ** argv )
{
  parse_and_handle_configuration( the::conf::ParameterVector( argv, argv + argc ) );
  const std::string home_folder( std::string( getenv( "HOME" ) ) + "/.yarrrserver/" );
  the::conf::set( "lua_configuration_path", home_folder );

  the::ctci::AutoServiceRegister< yarrr::ResourceFinder, yarrr::ResourceFinder > resource_finder_register(
      yarrr::ResourceFinder::PathList{
      home_folder,
      "/usr/local/share/yarrr/",
      "/usr/share/yarrr/" } );

  the::ctci::AutoServiceRegister< yarrrs::Notifier, yarrrs::Notifier > notifier_register(
      create_notification_stream() );

  yarrr::initialize_lua_engine();

  the::time::Clock clock;
  yarrrs::NetworkService network_service( clock );
  yarrr::ObjectContainer object_container;
  yarrrs::Player::Container players;
  yarrrs::World world( players, object_container );
  yarrrs::DuckHunt duck_hunt( object_container, clock );

  the::time::FrequencyStabilizer< 10, the::time::Clock > frequency_stabilizer( clock );
  while ( true )
  {
    network_service.process_network_events();
    object_container.dispatch( yarrr::TimerUpdate( clock.now() ) );
    object_container.check_collision();
    send_update_messages_from( object_container, players );
    frequency_stabilizer.stabilize();

    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
  }

  return 0;
}

